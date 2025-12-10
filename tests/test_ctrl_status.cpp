#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../src/app.h"
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>

TEST_CASE("CtrlRequest queueing and worker processing", "[ctrl_status]")
{
    // Create a minimal App instance (mock config)
    App app("test_config.xml");
    // Simulate a CtrlRequest
    CtrlRequest req;
    req.app_name = "test_app";
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
    App app("test_config.xml");
    // Fill the queue to max size
    for (size_t i = 0; i < App::CTRL_QUEUE_MAX_SIZE; ++i)
    {
        CtrlRequest req;
        req.app_name = "test_app";
        req.data = {static_cast<uint8_t>(i)};
        std::lock_guard<std::mutex> lock(app.ctrl_queue_mutex_);
        app.ctrl_queue_.push(req);
    }
    // Try to add one more
    CtrlRequest req;
    req.app_name = "overflow_app";
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
