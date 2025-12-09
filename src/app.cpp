#include "app.h"
#include <iostream>
#include <poll.h>
#include <csignal>
#include "icd.h"

App::App(const std::string &config_path)
    : config_(load_config(config_path.c_str())),
      udp_(config_.udp_local_port, config_.udp_remote_ip, config_.udp_remote_port)
{
    if (!udp_.bindSocket())
    {
        throw std::runtime_error("Error binding UDP socket");
    }

    // Create and bind all UDS servers (downlink)
    for (size_t i = 0; i < config_.uds_servers.size(); ++i)
    {
        const std::string &server_path = config_.uds_servers[i];
        std::unique_ptr<UdsSocket> server(new UdsSocket(server_path, ""));
        if (!server->bindSocket())
        {
            throw std::runtime_error("Error binding UDS server: " + server_path);
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
}

void App::run()
{
    std::cout << "App Service Running (XML Config).\n"
              << "UDP: " << config_.udp_local_port << " <-> " << config_.udp_remote_ip << ":" << config_.udp_remote_port << std::endl;
    std::cout << "UDS Servers (downlink):\n";
    for (const auto &s : config_.uds_servers)
        std::cout << "  " << s << std::endl;
    std::cout << "UDS Clients (uplink):\n";
    for (const auto &entry : config_.uds_clients)
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;

    // --- Polling and routing logic ---
    const size_t uds_count = uds_servers_.size();
    const size_t nfds = 1 + uds_count; // 1 for UDP, rest for UDS servers
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

    char buffer[4096];
    uint32_t msg_id_counter = 1;

    while (true)
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
                        client_it->second->send(buffer + sizeof(FslGslHeader), n - sizeof(FslGslHeader));
                    }
                    else
                    {
                        std::cerr << "No UDS client found for name: " << uds_name << std::endl;
                    }
                }
                else
                {
                    std::cerr << "No UDS mapping for opcode: " << opcode << std::endl;
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
                    hdr.msg_id = msg_id_counter++;
                    memcpy(buffer, &hdr, sizeof(FslGslHeader));
                    udp_.send(buffer, n + sizeof(FslGslHeader));
                }
            }
        }
    }
}
