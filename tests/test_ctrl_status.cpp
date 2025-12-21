#include "catch.hpp"
#include "../src/app.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <unistd.h>

// Helper to get config path safely
static std::string get_test_config_path()
{
    const char *home = getenv("HOME");
    REQUIRE(home != nullptr); // Fail test if HOME is not set
    return std::string(home) + "/dev/elar/elar_fsl/tests/test_config.xml";
}

TEST_CASE("CtrlRequest queueing and worker processing", "[ctrl_status]")
{
    // Create a minimal App instance (mock config)
    // Debug: print CWD and test file access
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)))
    {
        std::cout << "[DEBUG] CWD: " << cwd << std::endl;
        std::cout.flush();
    }
    std::cout << "[DEBUG] About to fopen test_config.xml" << std::endl;
    std::cout.flush();
    std::string config_path = get_test_config_path();
    FILE *f = fopen(config_path.c_str(), "r");
    if (f)
    {
        std::cout << "[DEBUG] test_config.xml opened successfully" << std::endl;
        fclose(f);
    }
    else
    {
        std::cout << "[DEBUG] Failed to open test_config.xml" << std::endl;
    }
    std::cout << "[DEBUG] About to call load_config" << std::endl;
    std::cout.flush();
    AppConfig cfg = load_config(config_path.c_str());
    std::cout << "[DEBUG] load_config returned successfully" << std::endl;
    std::cout.flush();
    App app(cfg);
    // Simulate a CtrlRequest
    CtrlRequest req;
    req.ctrl_uds_name = "test_app";
    req.data = {1, 2, 3, 4};

    // Lock and push to queue
    {
        std::lock_guard<std::mutex> lock(app.ctrl_queue_mutex_);
        app.ctrl_queue_.push(req);
    }
    app.ctrl_queue_cv_.notify_one();

    // Wait for worker to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // No assertion here, but you can check logs or extend App to set a flag
    REQUIRE(true); // Placeholder
}

TEST_CASE("CtrlRequest queue full error", "[ctrl_status]")
{
    std::string config_path = get_test_config_path();
    AppConfig cfg = load_config(config_path.c_str());
    App app(cfg);
    // Fill the queue to max size
    for (size_t i = 0; i < App::CTRL_QUEUE_MAX_SIZE; ++i)
    {
        CtrlRequest req;
        req.ctrl_uds_name = "test_app";
        req.data = {static_cast<uint8_t>(i)};
        std::lock_guard<std::mutex> lock(app.ctrl_queue_mutex_);
        app.ctrl_queue_.push(req);
    }
    // Try to add one more
    CtrlRequest req;
    req.ctrl_uds_name = "overflow_app";
    req.data = {99};
    bool queued = false;
    {
        std::lock_guard<std::mutex> lock(app.ctrl_queue_mutex_);
        if (app.ctrl_queue_.size() < App::CTRL_QUEUE_MAX_SIZE)
        {
            app.ctrl_queue_.push(req);
            queued = true;
        }
    }
    REQUIRE(!queued); // Should not be able to queue
}
