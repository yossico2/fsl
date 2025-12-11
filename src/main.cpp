
#include <iostream>
#include <csignal>
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
        AppConfig config = load_config("config.xml");
        App app(config);
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "App error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
