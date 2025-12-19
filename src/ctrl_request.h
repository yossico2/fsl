// ctrl_request.h - CtrlRequest struct for FSL ctrl/status queue
#pragma once
#include <string>
#include <vector>

// CtrlRequest: Control/status message for FSL ctrl queue
struct CtrlRequest
{
    std::string ctrl_uds_name; // Name of ctrl/status UDS channel
    std::vector<uint8_t> data; // Raw message data
    // Add more fields as needed for ctrl/status message
};
