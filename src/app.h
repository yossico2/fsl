// app.h - Main FSL Application class
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include "uds.h"    // For AppConfig and CtrlUdsConfig
#include "config.h" // For UdpServerSocket
#include "udp.h"    // For std::queue, std::mutex, std::condition_variable
#include <queue>
#include <mutex>
#include <condition_variable>
#include <csignal>        // For sig_atomic_t
#include "ctrl_request.h" // For CtrlRequest type

//
// The App class manages UDP and multiple UDS sockets (server/client) for routing
// messages between the ground segment (GSL) and space segment applications.
//
// Responsibilities:
//   - Load configuration from XML
//   - Create and manage UDP and UDS sockets
//   - Route messages based on opcode and UDS mapping
//   - Validate configuration and handle errors
//   - Support graceful shutdown via signal handling
//
// Main methods:
//   - App(const std::string &config_path): Constructor, loads config and sets up sockets
//   - run(): Main event loop for polling and routing
//   - cleanup(): Closes sockets and unlinks UDS files
//   - signalHandler(int): Handles SIGINT/SIGTERM for graceful shutdown

class App
{
public:
    static constexpr size_t CTRL_QUEUE_MAX_SIZE = 32;
    std::queue<CtrlRequest> ctrl_queue_;
    std::mutex ctrl_queue_mutex_;
    std::condition_variable ctrl_queue_cv_;
    App(const AppConfig &config);
    void run();
    void cleanup();
    static void signalHandler(int signum);
    void processCtrlRequest(const CtrlRequest &req);
    void processFSWCtrlRequest(std::vector<uint8_t> &data);
    void processPLMGCtrlRequest(std::vector<uint8_t> &data);
    void processELCtrlRequest(std::vector<uint8_t> &data);

    int processDownlinkMessage(const std::string &server_name, std::vector<uint8_t> &data, uint32_t &msg_id_counter);
    int processFSWDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);
    int processPLMGDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);
    int processELDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);

private:
    AppConfig config_;
    UdpServerSocket udp_;
    std::vector<std::unique_ptr<UdsSocket>> uds_servers_;
    std::map<std::string, std::unique_ptr<UdsSocket>> uds_clients_;
    struct CtrlUdsSockets
    {
        std::unique_ptr<UdsSocket> request;
        std::unique_ptr<UdsSocket> response;
    };
    std::map<std::string, CtrlUdsSockets> ctrl_uds_sockets_;
    static volatile sig_atomic_t shutdown_flag_;
};
