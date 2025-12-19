#include "catch.hpp"
#include "../src/app.h"
#include "../src/icd/fsl.h"
#include <iostream>
#include <vector>
#include <cstring>

// Helper to create a FSW ctrl request
static std::vector<uint8_t> make_fsw_ctrl_req(FslCtrlOpcode opcode, uint32_t seq_id = 1)
{
    FslCtrlGeneralRequest req = {};
    req.header.ctrl_opcode = opcode;
    req.header.ctrl_error_code = FSL_CTRL_ERR_NONE;
    req.header.ctrl_length = 0;
    req.header.ctrl_seq_id = seq_id;
    std::vector<uint8_t> data(sizeof(FslCtrlGeneralRequest));
    memcpy(data.data(), &req, sizeof(FslCtrlGeneralRequest));
    return data;
}

// Helper to parse a CBIT response
static FslCtrlGetCbitResponse parse_cbit_resp(const std::vector<uint8_t> &data)
{
    FslCtrlGetCbitResponse resp;
    REQUIRE(data.size() == sizeof(FslCtrlGetCbitResponse));
    memcpy(&resp, data.data(), sizeof(FslCtrlGetCbitResponse));
    return resp;
}

// Helper to parse a general response
static FslCtrlGeneralResponse parse_gen_resp(const std::vector<uint8_t> &data)
{
    FslCtrlGeneralResponse resp;
    REQUIRE(data.size() == sizeof(FslCtrlGeneralResponse));
    memcpy(&resp, data.data(), sizeof(FslCtrlGeneralResponse));
    return resp;
}

TEST_CASE("FSL state changes to OPER and STANDBY via FSW ctrl requests", "[ctrl_status]")
{
    AppConfig cfg = load_config("/home/yossico/dev/elar/elar_fsl/tests/test_config.xml");
    App app(cfg);

    // Simulate FSW ctrl response socket
    std::vector<uint8_t> last_response;
    struct DummyUdsSocket : public UdsSocket
    {
        std::vector<uint8_t> *out;
        DummyUdsSocket(std::vector<uint8_t> *o) : UdsSocket("", ""), out(o) {}
        ssize_t send(const void *buf, size_t len)
        {
            out->assign((const uint8_t *)buf, (const uint8_t *)buf + len);
            return len;
        }
    };

    struct AppTestFriend : public App
    {
        using App::ctrl_uds_sockets_;
        AppTestFriend(const AppConfig &c) : App(c) {}
    };
    AppTestFriend &app_friend = static_cast<AppTestFriend &>(app);
    app_friend.ctrl_uds_sockets_["FSW"].response = nullptr;
    app_friend.ctrl_uds_sockets_["FSW"].response.reset(new DummyUdsSocket(&last_response));

    // Set OPER
    auto req_oper = make_fsw_ctrl_req(FSL_CTRL_OP_SET_OPER, 42);
    app.processFSWCtrlRequest(req_oper);
    auto resp_oper = parse_gen_resp(last_response);
    REQUIRE(resp_oper.header.ctrl_opcode == FSL_CTRL_OP_SET_OPER);
    REQUIRE(resp_oper.header.ctrl_error_code == FSL_CTRL_ERR_NONE);
    // Check CBIT state is OPER
    auto req_cbit = make_fsw_ctrl_req(FSL_CTRL_OP_GET_CBIT, 43);
    app.processFSWCtrlRequest(req_cbit);
    auto resp_cbit = parse_cbit_resp(last_response);
    REQUIRE(resp_cbit.header.ctrl_opcode == FSL_CTRL_OP_GET_CBIT);
    REQUIRE(resp_cbit.state == FSL_STATE_OPER);

    // Set STANDBY
    auto req_standby = make_fsw_ctrl_req(FSL_CTRL_OP_SET_STANDBY, 44);
    app.processFSWCtrlRequest(req_standby);
    auto resp_standby = parse_gen_resp(last_response);
    REQUIRE(resp_standby.header.ctrl_opcode == FSL_CTRL_OP_SET_STANDBY);
    REQUIRE(resp_standby.header.ctrl_error_code == FSL_CTRL_ERR_NONE);
    // Check CBIT state is STANDBY
    req_cbit = make_fsw_ctrl_req(FSL_CTRL_OP_GET_CBIT, 45);
    app.processFSWCtrlRequest(req_cbit);
    resp_cbit = parse_cbit_resp(last_response);
    REQUIRE(resp_cbit.header.ctrl_opcode == FSL_CTRL_OP_GET_CBIT);
    REQUIRE(resp_cbit.state == FSL_STATE_STANDBY);
}
