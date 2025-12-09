#pragma once
#include <string>

struct AppConfig {
    int udp_local_port;
    std::string udp_remote_ip;
    int udp_remote_port;
    std::string uds_my_path;
    std::string uds_target_path;
};

AppConfig load_config(const char* filename);