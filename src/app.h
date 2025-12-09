#pragma once
#include <string>
#include "config.h"
#include "udp.h"
#include "uds.h"
#include <memory>
#include <signal.h>

class App
{
public:
    App(const std::string &config_path);
    void run();
    void cleanup();
    static void signalHandler(int signum);

private:
    AppConfig config_;
    UdpServerSocket udp_;
    std::vector<std::unique_ptr<UdsSocket>> uds_servers_;
    std::map<std::string, std::unique_ptr<UdsSocket>> uds_clients_;
    static volatile sig_atomic_t shutdown_flag_;
};
