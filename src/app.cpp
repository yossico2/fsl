// app.cpp - Implementation of the FSL Application
//
// This file implements the App class, which manages UDP and UDS sockets for routing
// messages between the ground segment (GSL) and space segment applications.
//
// Key features:
//   - Loads configuration from XML (see config.xml)
//   - Validates UDS mapping and socket paths
//   - Creates and binds UDP/UDS sockets
//   - Routes messages based on opcode and UDS mapping
//   - Logs message routing and errors
//   - Handles graceful shutdown via SIGINT/SIGTERM
//   - Cleans up sockets and UDS files on exit
//
// Main event loop: Uses poll() to wait for UDP and UDS events, routes messages accordingly.
// Error handling: Prints errors for invalid config, socket failures, and message routing issues.

#include "app.h"
#include <iostream>
#include <poll.h>
#include <csignal>
#include <signal.h>
#include <unistd.h>
#include <set>

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "logger.h"
#include "icd/fsl.h"
#include "icd/fcom.h"
#include "ctrl_request.h"

volatile sig_atomic_t App::shutdown_flag_ = 0;

constexpr size_t CTRL_QUEUE_MAX_SIZE = 32; // Adjust as needed
std::queue<CtrlRequest> ctrl_queue_;
std::mutex ctrl_queue_mutex_;
std::condition_variable ctrl_queue_cv_;
std::thread ctrl_worker_;
bool ctrl_worker_running_ = true;

App::App(const AppConfig &config)
    : config_(config),
      udp_(config_.udp_local_port, config_.udp_remote_ip, config_.udp_remote_port)
{
    // Set logger level from config
    auto toLogLevel = [](const std::string &lvl) -> LogLevel
    {
        if (lvl == "DEBUG")
            return LogLevel::DEBUG;
        if (lvl == "INFO")
            return LogLevel::INFO;
        return LogLevel::ERROR;
    };

    Logger::setLevel(toLogLevel(config.logging_level));

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, App::signalHandler);
    std::signal(SIGTERM, App::signalHandler);

    if (!udp_.bindSocket())
    {
        throw std::runtime_error("Error binding UDP socket");
    }

    // --- Configuration Validation ---
    // 1. Check all UDS mapping names exist in <client>
    for (const auto &mapping : config_.ul_uds_mapping)
    {
        const std::string &ctrl_uds_name = mapping.second;
        if (config_.uds_clients.find(ctrl_uds_name) == config_.uds_clients.end())
            Logger::error("UDS mapping name '" + ctrl_uds_name + "' (opcode " + std::to_string(mapping.first) + ") does not exist in <client> list.");
    }

    // 2. Ensure all UDS server/client paths are non-empty and unique
    std::set<std::string> uds_paths;
    for (const auto &server : config_.uds_servers)
    {
        if (server.path.empty())
            Logger::error("UDS server path is empty.");
        if (!uds_paths.insert(server.path).second)
            Logger::error("Duplicate UDS server path: '" + server.path + "'");
    }

    for (const auto &client : config_.uds_clients)
    {
        if (client.second.empty())
            Logger::error("UDS client '" + client.first + "' path is empty.");
        if (!uds_paths.insert(client.second).second)
            Logger::error("Duplicate UDS client path: '" + client.second + "' (name: " + client.first + ")");
    }

    // Create and bind all UDS servers (downlink)
    for (const auto &server_cfg : config_.uds_servers)
    {
        std::unique_ptr<UdsSocket> server(new UdsSocket(server_cfg.path, ""));
        if (server_cfg.receive_buffer_size > 0)
        {
            server->setReceiveBufferSize(server_cfg.receive_buffer_size);
        }
        if (!server->bindSocket())
        {
            throw std::runtime_error("Error binding UDS server: " + server_cfg.path);
        }
        uds_servers_.push_back(std::move(server));
    }

    // Create all UDS clients (uplink)
    for (std::map<std::string, std::string>::const_iterator it = config_.uds_clients.begin(); it != config_.uds_clients.end(); ++it)
    {
        const std::string &name = it->first;
        const std::string &path = it->second;
        std::unique_ptr<UdsSocket> client(new UdsSocket("", path));
        uds_clients_[name] = std::move(client);
    }

    // Create ctrl/status UDS sockets for each app
    for (const auto &entry : config_.ctrl_uds_name)
    {
        const std::string &ctrl_uds_name = entry.first;
        const CtrlUdsConfig &cfg = entry.second;
        CtrlUdsSockets sockets;
        if (!cfg.request_path.empty())
        {
            sockets.request.reset(new UdsSocket(cfg.request_path, ""));
            if (cfg.request_buffer_size > 0)
            {
                sockets.request->setReceiveBufferSize(cfg.request_buffer_size);
            }
            if (!sockets.request->bindSocket())
            {
                throw std::runtime_error("Error binding ctrl request UDS for " + ctrl_uds_name + ": " + cfg.request_path);
            }
        }
        if (!cfg.response_path.empty())
        {
            sockets.response.reset(new UdsSocket(cfg.response_path, ""));
            if (cfg.response_buffer_size > 0)
            {
                sockets.response->setReceiveBufferSize(cfg.response_buffer_size);
            }
            if (!sockets.response->bindSocket())
            {
                throw std::runtime_error("Error binding ctrl response UDS for " + ctrl_uds_name + ": " + cfg.response_path);
            }
        }
        ctrl_uds_sockets_[ctrl_uds_name] = std::move(sockets);
    }
}

void App::cleanup()
{
    // Close UDP socket
    // (UdpServerSocket destructor will close fd_)

    // Close and unlink UDS server sockets
    for (auto &server : uds_servers_)
    {
        const std::string &path = server->getMyPath();
        server.reset(); // Close socket
        if (!path.empty())
        {
            if (unlink(path.c_str()) == 0)
                Logger::info("Unlinked UDS file: " + path);
            else
                Logger::error("Failed to unlink UDS file: " + path + " (" + ::strerror(errno) + ")");
        }
    }
    uds_servers_.clear();

    // Close UDS client sockets
    for (auto &client : uds_clients_)
    {
        client.second.reset();
    }
    uds_clients_.clear();

    // Close and unlink ctrl/status UDS sockets
    for (auto &entry : ctrl_uds_sockets_)
    {
        auto &sockets = entry.second;
        if (sockets.request)
        {
            const std::string &path = sockets.request->getMyPath();
            sockets.request.reset();
            if (!path.empty())
            {
                if (unlink(path.c_str()) == 0)
                    Logger::info("Unlinked ctrl request UDS file: " + path);
                else
                    Logger::error("Failed to unlink ctrl request UDS file: " + path + " (" + ::strerror(errno) + ")");
            }
        }
        if (sockets.response)
        {
            const std::string &path = sockets.response->getMyPath();
            sockets.response.reset();
            if (!path.empty())
            {
                if (unlink(path.c_str()) == 0)
                    Logger::info("Unlinked ctrl response UDS file: " + path);
                else
                    Logger::error("Failed to unlink ctrl response UDS file: " + path + " (" + ::strerror(errno) + ")");
            }
        }
    }

    // Stop ctrl worker thread
    ctrl_worker_running_ = false;
    ctrl_queue_cv_.notify_one();

    if (ctrl_worker_.joinable())
        ctrl_worker_.join();
}

void App::signalHandler(int signum)
{
    shutdown_flag_ = 1;
}

void App::run()
{
    Logger::info("App Service Running (XML Config). UDP: " + std::to_string(config_.udp_local_port) + " <-> " + config_.udp_remote_ip + ":" + std::to_string(config_.udp_remote_port));
    std::string uds_servers_list = "UDS Servers (downlink):\n";
    for (const auto &s : config_.uds_servers)
        uds_servers_list += "  " + s.path + "\n";
    Logger::info(uds_servers_list);
    std::string uds_clients_list = "UDS Clients (uplink):\n";
    for (const auto &entry : config_.uds_clients)
        uds_clients_list += "  " + entry.first + ": " + entry.second + "\n";
    Logger::info(uds_clients_list);
    std::string ctrl_status_uds = "Ctrl/Status UDS:\n";
    for (const auto &entry : config_.ctrl_uds_name)
    {
        const std::string &app = entry.first;
        const CtrlUdsConfig &c = entry.second;
        ctrl_status_uds += "  [" + app + "]";
        if (!c.request_path.empty())
            ctrl_status_uds += " request: " + c.request_path;
        if (!c.response_path.empty())
            ctrl_status_uds += " response: " + c.response_path;
        ctrl_status_uds += "\n";
    }
    Logger::info(ctrl_status_uds);

    // === Start ctrl worker thread
    ctrl_worker_running_ = true;
    ctrl_worker_ = std::thread([this]
                               {
        while (ctrl_worker_running_) {
            std::unique_lock<std::mutex> lock(ctrl_queue_mutex_);
            ctrl_queue_cv_.wait(lock, [this]{ return !ctrl_queue_.empty() || !ctrl_worker_running_; });
            while (!ctrl_queue_.empty()) {
                CtrlRequest req = std::move(ctrl_queue_.front());
                ctrl_queue_.pop();
                lock.unlock();
                processCtrlRequest(req);
                lock.lock();
            }
        } });

    // === Polling and routing logic ---
    // Layout: [0]=UDP, [1..N]=UDS servers, [N+1..]=ctrl_uds_sockets_ (request only)
    const size_t uds_count = uds_servers_.size();
    std::vector<std::string> ctrl_uds_names;
    for (const auto &entry : ctrl_uds_sockets_)
    {
        if (entry.second.request)
            ctrl_uds_names.push_back(entry.first);
    }

    const size_t nfds = 1 + uds_count + ctrl_uds_names.size();
    std::vector<pollfd> fds(nfds);

    // UDP socket
    fds[0].fd = udp_.getFd();
    fds[0].events = POLLIN;

    // UDS server sockets
    for (size_t i = 0; i < uds_count; ++i)
    {
        fds[1 + i].fd = uds_servers_[i]->getFd();
        fds[1 + i].events = POLLIN;
    }

    // ctrl_uds_sockets_ (requests only)
    for (size_t i = 0; i < ctrl_uds_names.size(); ++i)
    {
        const auto &ctrl_uds_name = ctrl_uds_names[i];
        const auto &sockets = ctrl_uds_sockets_.at(ctrl_uds_name);
        int fd = -1;
        if (sockets.request)
            fd = sockets.request->getFd();
        fds[1 + uds_count + i].fd = fd;
        fds[1 + uds_count + i].events = POLLIN;
    }

    char buffer[4096];
    uint32_t msg_id_counter = 1;

    while (!shutdown_flag_)
    {
        int ret = poll(fds.data(), nfds, -1);
        if (ret < 0)
        {
            Logger::error(std::string("Poll failed: ") + ::strerror(errno));
            break;
        }

        // --- UDP -> UDS client (uplink) ---
        if (fds[0].revents & POLLIN)
        {
            sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            int n = udp_.receive(buffer, sizeof(buffer), &sender_addr);
            if (n >= (int)sizeof(GslFslHeader))
            {
                const GslFslHeader *hdr = reinterpret_cast<const GslFslHeader *>(buffer);
                uint16_t opcode = hdr->opcode; // opcode is actually destination for uplink
                std::map<uint16_t, std::string>::const_iterator map_it = config_.ul_uds_mapping.find(opcode);
                if (map_it != config_.ul_uds_mapping.end())
                {
                    const std::string &ctrl_uds_name = map_it->second;
                    std::map<std::string, std::unique_ptr<UdsSocket>>::iterator client_it = uds_clients_.find(ctrl_uds_name);
                    if (client_it != uds_clients_.end())
                    {
                        // Forward only the payload (excluding header)
                        ssize_t sent = client_it->second->send(buffer + sizeof(GslFslHeader), n - sizeof(GslFslHeader));
                        if (sent < 0)
                        {
                            Logger::error("Failed to send to UDS client '" + ctrl_uds_name + "' (opcode: " + std::to_string(opcode) + ")");
                        }
                        else
                        {
                            if (Logger::isDebugEnabled())
                            {
                                Logger::debug("Routed UDP->UDS: opcode=" + std::to_string(opcode) + ", bytes=" + std::to_string(sent) + ", dest='" + ctrl_uds_name + "'");
                            }
                        }
                    }
                    else
                    {
                        Logger::error("No UDS client found for name: " + ctrl_uds_name);
                    }
                }
                else
                {
                    Logger::error("No UDS mapping for opcode: " + std::to_string(opcode));
                }
            }
        }

        // --- UDS server(s) -> UDP (downlink) ---
        for (size_t i = 0; i < uds_count; ++i)
        {
            if (fds[1 + i].revents & POLLIN)
            {
                int n = uds_servers_[i]->receive(buffer + sizeof(GslFslHeader), sizeof(buffer) - sizeof(GslFslHeader));
                if (n > 0)
                {
                    std::string server_name;
                    if (i < config_.uds_servers.size())
                        server_name = config_.uds_servers[i].name;

                    std::vector<uint8_t> downlink_data(buffer + sizeof(GslFslHeader), buffer + sizeof(GslFslHeader) + n);

                    int sent = processDownlinkMessage(server_name, downlink_data, msg_id_counter);
                    if (sent < 0)
                    {
                        Logger::error("Failed to send UDP packet from UDS server index " + std::to_string(i));
                    }
                    else
                    {
                        if (Logger::isDebugEnabled())
                        {
                            Logger::debug("Routed UDS->UDP: bytes=" + std::to_string(sent) + ", src='" + uds_servers_[i]->getMyPath() + "' (server: '" + server_name + "')");
                        }
                    }
                }
                else if (n < 0)
                {
                    Logger::error("Failed to receive from UDS server index " + std::to_string(i));
                }
            }
        }

        // --- ctrl_uds_sockets_ (request only) handlers ---
        for (size_t i = 0; i < ctrl_uds_names.size(); ++i)
        {
            if (fds[1 + uds_count + i].revents & POLLIN)
            {
                const std::string &ctrl_uds_name = ctrl_uds_names[i];
                int n = ctrl_uds_sockets_[ctrl_uds_name].request->receive(buffer, sizeof(buffer));
                if (n > 0)
                {
                    if (Logger::isDebugEnabled())
                    {
                        Logger::debug("[CTRL] Received request for '" + ctrl_uds_name + "', bytes=" + std::to_string(n));
                    }
                    // Producer: enqueue ctrl request for worker thread
                    CtrlRequest req;
                    req.ctrl_uds_name = ctrl_uds_name;
                    req.data.assign(buffer, buffer + n);
                    bool queued = false;
                    {
                        std::lock_guard<std::mutex> lock(ctrl_queue_mutex_);
                        if (ctrl_queue_.size() < CTRL_QUEUE_MAX_SIZE)
                        {
                            ctrl_queue_.push(std::move(req));
                            queued = true;
                        }
                    }
                    if (queued)
                    {
                        ctrl_queue_cv_.notify_one();
                    }
                    else
                    {
                        // Buffer full: handle error (log, respond, etc.)
                        Logger::error("[CTRL] Queue full, dropping request for '" + ctrl_uds_name + "'");
                        // TODO: Optionally send FSL_CTRL_ERR_QUEUE_FULL response to client
                    }
                }
                else if (n < 0)
                {
                    Logger::error("[CTRL] Failed to receive request for '" + ctrl_uds_name + "'");
                }
            }
        }
    }

    cleanup();
    Logger::info("Graceful shutdown complete.");
}

void App::processCtrlRequest(const CtrlRequest &req)
{
    if (req.ctrl_uds_name == "FSW")
    {
        processFSWCtrlRequest(const_cast<std::vector<uint8_t> &>(req.data));
    }
    else if (req.ctrl_uds_name == "PLMG")
    {
        processPLMGCtrlRequest(const_cast<std::vector<uint8_t> &>(req.data));
    }
    else if (req.ctrl_uds_name == "EL")
    {
        processELCtrlRequest(const_cast<std::vector<uint8_t> &>(req.data));
    }
    else
    {
        Logger::error("[CTRL-WORKER] Unknown ctrl_uds_name: '" + req.ctrl_uds_name + "'");
    }
}

void App::processFSWCtrlRequest(std::vector<uint8_t> &data)
{
    // Handle FSW control request
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] Processing FSW control request, bytes=" + std::to_string(data.size()));
    }
    const fcom_fsw_CS_Header *hdr = reinterpret_cast<const fcom_fsw_CS_Header *>(data.data());
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] FSW Header: opcode=" + std::to_string(hdr->opcode) +
                      ", length=" + std::to_string(hdr->length) +
                      ", seq_id=" + std::to_string(hdr->seq_id));
    }
    // lilo:TODO: Implement FSW control request handling
}

void App::processPLMGCtrlRequest(std::vector<uint8_t> &data)
{
    // Handle PLMG control request
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] Processing PLMG control request, bytes=" + std::to_string(data.size()));
    }
    const plmg_fcom_Header *hdr = reinterpret_cast<const plmg_fcom_Header *>(data.data());
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] PLMG Header: opcode=" + std::to_string(hdr->opcode) +
                      ", length=" + std::to_string(hdr->length) +
                      ", seq_id=" + std::to_string(hdr->seq_id));
    }
    // lilo:TODO: Implement PLMG control request handling
}

void App::processELCtrlRequest(std::vector<uint8_t> &data)
{
    // Handle EL control request
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] Processing EL control request, bytes=" + std::to_string(data.size()));
    }
    const plmg_fcom_Header *hdr = reinterpret_cast<const plmg_fcom_Header *>(data.data());
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[CTRL] EL Header: opcode=" + std::to_string(hdr->opcode) +
                      ", length=" + std::to_string(hdr->length) +
                      ", seq_id=" + std::to_string(hdr->seq_id));
    }
    // lilo:TODO: Implement EL control request handling
}

// --- Downlink message router ---
// Returns number of bytes sent, or <0 on error
int App::processDownlinkMessage(const std::string &server_name, std::vector<uint8_t> &data, uint32_t &msg_id_counter)
{
    if (server_name == "FSW_HIGH_DL" || server_name == "FSW_LOW_DL")
        return processFSWDownlink(data, msg_id_counter);

    if (server_name == "DL_PLMG_H" || server_name == "DL_PLMG_L")
        return processPLMGDownlink(data, msg_id_counter);

    if (server_name == "DL_EL_H" || server_name == "DL_EL_L")
        return processELDownlink(data, msg_id_counter);

    return -1;
}

// --- Downlink handlers ---
// Returns number of bytes sent, or <0 on error
int App::processFSWDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter)
{
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[DOWNLINK] Processing FSW downlink, bytes=" + std::to_string(data.size()));
    }

    // FSW: no header, just payload
    char buffer[4096];
    GslFslHeader hdr;
    hdr.opcode = 0; // No opcode in payload
    hdr.length = data.size();
    hdr.seq_id = msg_id_counter++;
    memcpy(buffer, &hdr, sizeof(GslFslHeader));
    memcpy(buffer + sizeof(GslFslHeader), data.data(), data.size());
    return udp_.send(buffer, data.size() + sizeof(GslFslHeader));
}

// Returns number of bytes sent, or <0 on error
int App::processPLMGDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter)
{
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[DOWNLINK] Processing PLMG downlink, bytes=" + std::to_string(data.size()));
    }

    // PLMG: header + payload, header is plmg_fcom_Header
    if (data.size() < sizeof(plmg_fcom_Header))
    {
        Logger::error("PLMG downlink too short for header");
        return -1;
    }

    const plmg_fcom_Header *hdr_in = reinterpret_cast<const plmg_fcom_Header *>(data.data());
    uint16_t opcode = hdr_in->opcode;
    const uint8_t *payload = data.data() + sizeof(plmg_fcom_Header);
    size_t payload_len = data.size() - sizeof(plmg_fcom_Header);
    char buffer[4096];
    GslFslHeader hdr;
    hdr.opcode = opcode;
    hdr.length = payload_len;
    hdr.seq_id = msg_id_counter++;
    memcpy(buffer, &hdr, sizeof(GslFslHeader));
    memcpy(buffer + sizeof(GslFslHeader), payload, payload_len);
    return udp_.send(buffer, payload_len + sizeof(GslFslHeader));
}

// Returns number of bytes sent, or <0 on error
int App::processELDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter)
{
    if (Logger::isDebugEnabled())
    {
        Logger::debug("[DOWNLINK] Processing EL downlink, bytes=" + std::to_string(data.size()));
    }

    // EL: header + payload, header is plmg_fcom_Header
    if (data.size() < sizeof(plmg_fcom_Header))
    {
        Logger::error("EL downlink too short for header");
        return -1;
    }

    const plmg_fcom_Header *hdr_in = reinterpret_cast<const plmg_fcom_Header *>(data.data());
    uint16_t opcode = hdr_in->opcode;
    const uint8_t *payload = data.data() + sizeof(plmg_fcom_Header);
    size_t payload_len = data.size() - sizeof(plmg_fcom_Header);
    char buffer[4096];
    GslFslHeader hdr;
    hdr.opcode = opcode;
    hdr.length = payload_len;
    hdr.seq_id = msg_id_counter++;
    memcpy(buffer, &hdr, sizeof(GslFslHeader));
    memcpy(buffer + sizeof(GslFslHeader), payload, payload_len);
    return udp_.send(buffer, payload_len + sizeof(GslFslHeader));
}
