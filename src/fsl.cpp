#include "fsl.h"
#include <iostream>
#include <poll.h>
#include <csignal>

FSL::FSL(const std::string &config_path)
    : config_(load_config(config_path.c_str())),
      udp_(config_.udp_local_port, config_.udp_remote_ip, config_.udp_remote_port),
      uds_(config_.uds_my_path, config_.uds_target_path)
{
    if (!udp_.bindSocket())
    {
        throw std::runtime_error("Error binding UDP socket");
    }
    if (!uds_.bindSocket())
    {
        throw std::runtime_error("Error binding UDS socket");
    }
}

void FSL::run()
{
    std::cout << "FSL Service Running (XML Config).\n"
              << "UDP: " << config_.udp_local_port << " <-> " << config_.udp_remote_ip << ":" << config_.udp_remote_port << "\n"
              << "UDS: " << config_.uds_my_path << " -> " << config_.uds_target_path << std::endl;

    struct pollfd fds[2];
    fds[0].fd = udp_.getFd();
    fds[0].events = POLLIN;
    fds[1].fd = uds_.getFd();
    fds[1].events = POLLIN;

    char buffer[4096];

    while (true)
    {
        int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            perror("Poll failed");
            break;
        }

        // UDP -> UDS
        if (fds[0].revents & POLLIN)
        {
            sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            int n = udp_.receive(buffer, sizeof(buffer), &sender_addr);
            if (n > 0)
            {
                uds_.send(buffer, n);
            }
        }

        // UDS -> UDP
        if (fds[1].revents & POLLIN)
        {
            int n = uds_.receive(buffer, sizeof(buffer));
            if (n > 0)
            {
                udp_.send(buffer, n);
            }
        }
    }
}
