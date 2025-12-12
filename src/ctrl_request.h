// ctrl_request.h - CtrlRequest struct for FSL ctrl/status queue
#pragma once
#include <string>
#include <vector>

struct CtrlRequest
{
    std::string ctrl_uds_name;
    std::vector<uint8_t> data;
    // Add more fields as needed for ctrl/status message
};
