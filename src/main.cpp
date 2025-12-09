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

#include "fsl.h"

using namespace tinyxml2;

void signal_handler(int signum)
{
    std::cout << "\nClosing application..." << std::endl;
    exit(signum);
}

int main()
{
    signal(SIGINT, signal_handler);
    try
    {
        FSL fsl("./config.xml");
        fsl.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "FSL error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
