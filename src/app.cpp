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
#include "icd.h"

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "ctrl_request.h"

volatile sig_atomic_t App::shutdown_flag_ = 0;

constexpr size_t CTRL_QUEUE_MAX_SIZE = 32; // Adjust as needed
std::queue<CtrlRequest> ctrl_queue_;
std::mutex ctrl_queue_mutex_;
std::condition_variable ctrl_queue_cv_;
std::thread ctrl_worker_;
bool ctrl_worker_running_ = true;

App::App(const std::string &config_path)
    : config_(load_config(config_path.c_str())),
      udp_(config_.udp_local_port, config_.udp_remote_ip, config_.udp_remote_port)
{
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
        const std::string &uds_name = mapping.second;
        if (config_.uds_clients.find(uds_name) == config_.uds_clients.end())
        {
            std::cerr << "[CONFIG ERROR] UDS mapping name '" << uds_name << "' (opcode " << mapping.first << ") does not exist in <client> list." << std::endl;
        }
    }

    // 2. Ensure all UDS server/client paths are non-empty and unique
    std::set<std::string> uds_paths;
    for (const auto &server : config_.uds_servers)
    {
        if (server.path.empty())
        {
            std::cerr << "[CONFIG ERROR] UDS server path is empty." << std::endl;
        }
        if (!uds_paths.insert(server.path).second)
        {
            std::cerr << "[CONFIG ERROR] Duplicate UDS server path: '" << server.path << "'" << std::endl;
        }
    }

    for (const auto &client : config_.uds_clients)
    {
        if (client.second.empty())
        {
            std::cerr << "[CONFIG ERROR] UDS client '" << client.first << "' path is empty." << std::endl;
        }
        if (!uds_paths.insert(client.second).second)
        {
            std::cerr << "[CONFIG ERROR] Duplicate UDS client path: '" << client.second << "' (name: " << client.first << ")" << std::endl;
        }
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

    // Create ctrl/status UDS sockets for each app/tod
    for (const auto &entry : config_.ctrl_uds)
    {
        const std::string &app_name = entry.first;
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
                throw std::runtime_error("Error binding ctrl request UDS for " + app_name + ": " + cfg.request_path);
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
                throw std::runtime_error("Error binding ctrl response UDS for " + app_name + ": " + cfg.response_path);
            }
        }
        ctrl_uds_sockets_[app_name] = std::move(sockets);
    }
}

void App::run()
{
    std::cout << "App Service Running (XML Config).\n"
              << "UDP: " << config_.udp_local_port << " <-> " << config_.udp_remote_ip << ":" << config_.udp_remote_port << std::endl;
    std::cout << "UDS Servers (downlink):\n";
    for (const auto &s : config_.uds_servers)
        std::cout << "  " << s.path << std::endl;
    std::cout << "UDS Clients (uplink):\n";
    for (const auto &entry : config_.uds_clients)
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;

    std::cout << "Ctrl/Status UDS:\n";
    for (const auto &entry : config_.ctrl_uds)
    {
        const std::string &app = entry.first;
        const CtrlUdsConfig &c = entry.second;
        std::cout << "  [" << app << "]";
        if (!c.request_path.empty())
            std::cout << " request: " << c.request_path;
        if (!c.response_path.empty())
            std::cout << " response: " << c.response_path;
        std::cout << std::endl;
    }

    // Start ctrl worker thread
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

    // --- Polling and routing logic ---
    // Layout: [0]=UDP, [1..N]=UDS servers, [N+1..]=ctrl_uds_sockets_ (request only)
    const size_t uds_count = uds_servers_.size();
    std::vector<std::string> ctrl_apps;
    for (const auto &entry : ctrl_uds_sockets_)
    {
        if (entry.second.request)
            ctrl_apps.push_back(entry.first);
    }

    const size_t nfds = 1 + uds_count + ctrl_apps.size();
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

    // ctrl_uds_sockets_ (request only)
    for (size_t i = 0; i < ctrl_apps.size(); ++i)
    {
        const auto &app_name = ctrl_apps[i];
        const auto &sockets = ctrl_uds_sockets_.at(app_name);
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
            perror("Poll failed");
            break;
        }

        // --- UDP -> UDS client (uplink) ---
        if (fds[0].revents & POLLIN)
        {
            sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            int n = udp_.receive(buffer, sizeof(buffer), &sender_addr);
            if (n >= (int)sizeof(FslGslHeader))
            {
                const FslGslHeader *hdr = reinterpret_cast<const FslGslHeader *>(buffer);
                uint16_t opcode = hdr->msg_opcode;
                std::map<uint16_t, std::string>::const_iterator map_it = config_.ul_uds_mapping.find(opcode);
                if (map_it != config_.ul_uds_mapping.end())
                {
                    const std::string &uds_name = map_it->second;
                    std::map<std::string, std::unique_ptr<UdsSocket>>::iterator client_it = uds_clients_.find(uds_name);
                    if (client_it != uds_clients_.end())
                    {
                        // Forward only the payload (excluding header)
                        ssize_t sent = client_it->second->send(buffer + sizeof(FslGslHeader), n - sizeof(FslGslHeader));
                        if (sent < 0)
                        {
                            std::cerr << "[ERROR] Failed to send to UDS client '" << uds_name << "' (opcode: " << opcode << ")" << std::endl;
                        }
                        else
                        {
                            std::cout << "[LOG] Routed UDP->UDS: opcode=" << opcode << ", bytes=" << sent << ", dest='" << uds_name << "'" << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "No UDS client found for name: " << uds_name << std::endl;
                    }
                }
                else
                {
                    std::cerr << "[ERROR] No UDS mapping for opcode: " << opcode << std::endl;
                }
            }
        }

        // --- UDS server(s) -> UDP (downlink) ---
        for (size_t i = 0; i < uds_count; ++i)
        {
            if (fds[1 + i].revents & POLLIN)
            {
                int n = uds_servers_[i]->receive(buffer + sizeof(FslGslHeader), sizeof(buffer) - sizeof(FslGslHeader));
                if (n > 0)
                {
                    FslGslHeader hdr;
                    hdr.msg_opcode = 0; // Downlink opcode
                    hdr.msg_length = n;
                    hdr.msg_seq_id = msg_id_counter++;
                    memcpy(buffer, &hdr, sizeof(FslGslHeader));
                    ssize_t sent = udp_.send(buffer, n + sizeof(FslGslHeader));
                    if (sent < 0)
                    {
                        std::cerr << "[ERROR] Failed to send UDP packet from UDS server index " << i << std::endl;
                    }
                    else
                    {
                        std::cout << "[LOG] Routed UDS->UDP: bytes=" << sent << ", src='" << uds_servers_[i]->getMyPath() << "'" << std::endl;
                    }
                }
                else if (n < 0)
                {
                    std::cerr << "[ERROR] Failed to receive from UDS server index " << i << std::endl;
                }
            }
        }

        // --- ctrl_uds_sockets_ (request only) handlers ---
        for (size_t i = 0; i < ctrl_apps.size(); ++i)
        {
            if (fds[1 + uds_count + i].revents & POLLIN)
            {
                const std::string &app_name = ctrl_apps[i];
                int n = ctrl_uds_sockets_[app_name].request->receive(buffer, sizeof(buffer));
                if (n > 0)
                {
                    std::cout << "[CTRL] Received request for '" << app_name << "', bytes=" << n << std::endl;
                    // Producer: enqueue ctrl request for worker thread
                    CtrlRequest req;
                    req.app_name = app_name;
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
                        std::cerr << "[CTRL] Queue full, dropping request for '" << app_name << "'" << std::endl;
                        // TODO: Optionally send FSL_CTRL_ERR_QUEUE_FULL response to client
                    }
                }
                else if (n < 0)
                {
                    std::cerr << "[CTRL] Failed to receive request for '" << app_name << "'" << std::endl;
                }
            }
        }
    }

    cleanup();
    std::cout << "[INFO] Graceful shutdown complete." << std::endl;
}

void App::processCtrlRequest(const CtrlRequest &req)
{
    // Actual ctrl message processing logic here
    std::cout << "[CTRL-WORKER] Processing request for '" << req.app_name << "', bytes=" << req.data.size() << std::endl;
    // TODO: Implement ctrl message handling
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
            {
                std::cout << "[INFO] Unlinked UDS file: " << path << std::endl;
            }
            else
            {
                perror(("[WARN] Failed to unlink UDS file: " + path).c_str());
            }
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
                {
                    std::cout << "[INFO] Unlinked ctrl request UDS file: " << path << std::endl;
                }
                else
                {
                    perror(("[WARN] Failed to unlink ctrl request UDS file: " + path).c_str());
                }
            }
        }
        if (sockets.response)
        {
            const std::string &path = sockets.response->getMyPath();
            sockets.response.reset();
            if (!path.empty())
            {
                if (unlink(path.c_str()) == 0)
                {
                    std::cout << "[INFO] Unlinked ctrl response UDS file: " << path << std::endl;
                }
                else
                {
                    perror(("[WARN] Failed to unlink ctrl response UDS file: " + path).c_str());
                }
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
