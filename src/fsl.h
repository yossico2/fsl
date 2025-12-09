#pragma once
#include <string>
#include "config.h"
#include "udp_server.h"
#include "uds_server.h"

class FSL
{
public:
    FSL(const std::string &config_path);
    void run();

private:
    AppConfig config_;
    UdpServerSocket udp_;
    UdsServerSocket uds_;
};
