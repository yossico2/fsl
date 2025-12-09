#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

struct AppConfig
{
    int udp_local_port;
    std::string udp_remote_ip;
    int udp_remote_port;

    // Downlink: UDS servers (one or more per app)
    std::vector<std::string> uds_servers;

    // Uplink: UDS clients (name -> path)
    std::map<std::string, std::string> uds_clients;

    // Uplink: opcode -> uds client name
    std::map<uint16_t, std::string> ul_uds_mapping;
};

AppConfig load_config(const char *filename);