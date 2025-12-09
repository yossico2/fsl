
#include <iostream>
#include <csignal>
#include "app.h"

#include "app.h"

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
        App fsl("config.xml");
        fsl.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "App error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
