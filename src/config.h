// config.h - Configuration structures and loader for FSL
//
// Defines the AppConfig struct, which holds all configuration parameters loaded from config.xml.
// Used by the App class to set up UDP and UDS sockets and routing.
//
// Fields:
//   - udp_local_port: Local UDP port for FSL
//   - udp_remote_ip: Remote IP address for UDP communication
//   - udp_remote_port: Remote UDP port
//   - uds_servers: List of UDS server socket paths (downlink)
//   - uds_clients: Map of UDS client names to paths (uplink)
//   - ul_uds_mapping: Map of message opcodes to UDS client names (for uplink routing)
//
// Function:
//   - load_config(const char *filename): Parses config.xml and returns AppConfig

#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Holds ctrl/status UDS paths for each app
struct CtrlUdsConfig
{
    std::string request_path;
    int request_buffer_size = 0;
    std::string response_path;
    int response_buffer_size = 0;
};

struct UdsServerConfig
{
    std::string path;
    int receive_buffer_size = 0;
};

struct AppConfig
{
    int udp_local_port;        ///< Local UDP port for FSL
    std::string udp_remote_ip; ///< Remote IP address for UDP communication
    int udp_remote_port;       ///< Remote UDP port

    // Downlink: UDS servers (one or more per each app)
    std::vector<UdsServerConfig> uds_servers;

    // Uplink: UDS clients (name -> path)
    std::map<std::string, std::string> uds_clients;

    // Uplink: opcode -> uds client name
    std::map<uint16_t, std::string> ul_uds_mapping;

    // Ctrl/Status: app_name -> CtrlUdsConfig
    std::map<std::string, CtrlUdsConfig> ctrl_uds;
};

AppConfig load_config(const char *filename); ///< Parses config.xml and returns AppConfig