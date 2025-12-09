#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

// Include TinyXML-2 header
// Ensure libtinyxml2-dev is installed on your PetaLinux rootfs
#include <tinyxml2.h>

#include "icd.h"
#include "config.h"
#include "udp_server.h"
#include "uds_server.h"

using namespace tinyxml2;

void signal_handler(int signum)
{
    std::cout << "\nClosing application..." << std::endl;
    exit(signum);
}

int main()
{
    signal(SIGINT, signal_handler);

    AppConfig config;

    try
    {
        config = load_config("./config.xml");
        std::cout << "Configuration loaded from XML successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return 1;
    }

    // ====================================================
    // Step 1: Initialize UDP Server Socket
    // ====================================================

    UdpServerSocket udp(config.udp_local_port, config.udp_remote_ip, config.udp_remote_port);
    if (!udp.bindSocket())
    {
        std::cerr << "Error binding UDP socket" << std::endl;
        return 1;
    }

    // ====================================================
    // Step 2: Initialize UDS Server Socket
    // ====================================================
    UdsServerSocket uds(config.uds_my_path, config.uds_target_path);
    if (!uds.bindSocket())
    {
        std::cerr << "Error binding UDS socket" << std::endl;
        return 1;
    }

    std::cout << "fsl service running (config.xml).\n"
              << "UDP: " << config.udp_local_port << " <-> " << config.udp_remote_ip << ":" << config.udp_remote_port << "\n"
              << "UDS: " << config.uds_my_path << " -> " << config.uds_target_path << std::endl;

    // ====================================================
    // Step 3: Main Loop
    // ====================================================
    struct pollfd fds[2];
    fds[0].fd = udp.getFd();
    fds[0].events = POLLIN;
    fds[1].fd = uds.getFd();
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
            int n = udp.receive(buffer, sizeof(buffer), &sender_addr);
            if (n > 0)
            {
                if (n < FSL_GSL_HEADER_SIZE)
                {
                    std::cerr << "Received UDP packet too small for FSL_GSL_HEADER_SIZE: " << n << " bytes" << std::endl;
                    continue;
                }

                // lilo:TODO: rout to different UDS based on msg_opcode
                FslGslHeader *header = reinterpret_cast<FslGslHeader *>(buffer);

                uds.send(buffer, n);
            }
        }

        // UDS -> UDP
        if (fds[1].revents & POLLIN)
        {
            int n = uds.receive(buffer, sizeof(buffer));
            if (n > 0)
            {
                udp.send(buffer, n);
            }
        }
    }

    return 0;
}
