#pragma once
#include <string>
#include "config.h"
#include "udp.h"
#include "uds.h"
#include <memory>

class App
{
public:
    App(const std::string &config_path);
    void run();

private:
    AppConfig config_;
    UdpServerSocket udp_;
    // Multiple UDS servers (downlink)
    std::vector<std::unique_ptr<UdsSocket>> uds_servers_;
    // Multiple UDS clients (uplink), mapped by name
    std::map<std::string, std::unique_ptr<UdsSocket>> uds_clients_;
};
