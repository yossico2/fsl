// For test access
#ifdef UNIT_TEST_FRIENDS
friend struct AppTestFriend;
#endif
// app.h - Main FSL Application class
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include "config.h"
#include "uds.h"
#include "udp.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <csignal>   // For sig_atomic_t
#include "icd/fsl.h" // For FslStates, FslCtrl* types

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

// CtrlRequest: Control/status message for FSL ctrl queue
struct CtrlRequest
{
    std::string ctrl_uds_name; // Name of ctrl/status UDS channel
    std::vector<uint8_t> data; // Raw message data
};

class App
{
public:
    static constexpr size_t CTRL_QUEUE_MAX_SIZE = 32;
    std::queue<CtrlRequest> ctrl_queue_;
    std::mutex ctrl_queue_mutex_;
    std::condition_variable ctrl_queue_cv_;

    // Constructor: Loads config and sets up sockets
    App(const AppConfig &config);

    // Main event loop for polling and routing
    void run();

    // Closes sockets and unlinks UDS files
    void cleanup();

    // Handles SIGINT/SIGTERM for graceful shutdown
    static void signalHandler(int signum);

    // Process a control request from the ctrl queue
    void processCtrlRequest(const CtrlRequest &req);

    // Process FSW control request
    void processFSWCtrlRequest(std::vector<uint8_t> &data);

    // Process PLMG control request
    void processPLMGCtrlRequest(std::vector<uint8_t> &data);

    // Process EL control request
    void processELCtrlRequest(std::vector<uint8_t> &data);

    // Process a downlink message for a given server
    int processDownlinkMessage(const std::string &server_name, std::vector<uint8_t> &data, uint32_t &msg_id_counter);

    // Process FSW downlink message
    int processFSWDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);

    // Process PLMG downlink message
    int processPLMGDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);

    // Process EL downlink message
    int processELDownlink(std::vector<uint8_t> &data, uint32_t &msg_id_counter);

protected:
    AppConfig config_;
    UdpServerSocket udp_;
    std::vector<std::unique_ptr<UdsSocket>> uds_servers_;
    std::map<std::string, std::unique_ptr<UdsSocket>> uds_clients_;
    struct CtrlUdsSockets
    {
        std::unique_ptr<UdsSocket> request;
        std::unique_ptr<UdsSocket> response;
        // Flag for graceful shutdown (set by signal handler)
        static volatile std::sig_atomic_t shutdown_flag_;
    };
    std::map<std::string, CtrlUdsSockets> ctrl_uds_sockets_;
    static volatile sig_atomic_t shutdown_flag_;

    // CBIT state for FSL (for PLMG ctrl requests)
    FslStates cbit_state_ = FSL_STATE_STANDBY;
};
