#pragma once

#include <cstdint>
#include <cstddef>

/// FSL-GSL protocol message header (UDP framing)
typedef struct FslGslHeader
{
    uint16_t msg_opcode; ///< Message opcode (application-specific)
    uint16_t msg_length; ///< Payload length (bytes)
    uint32_t msg_seq_id; ///< Unique message ID (monotonic)
} FslGslHeader;

/// Size of FslGslHeader struct (for framing)
static const size_t FSL_GSL_HEADER_SIZE = sizeof(FslGslHeader);

/// FSL operational states
enum FslStates : uint8_t
{
    FSL_STATE_STANDBY = 0, ///< Standby state
    FSL_STATE_OPER = 1,    ///< Operational state
};

/// Control opcodes for FSL ctrl/status protocol
enum FslCtrlOpcode : uint8_t
{
    FSL_CTRL_OP_NOP = 0,         ///< No operation
    FSL_CTRL_OP_GET_CBIT = 1,    ///< Query CBIT/status
    FSL_CTRL_OP_SET_OPER = 2,    ///< Set FSL to OPER state
    FSL_CTRL_OP_SET_STANDBY = 3, ///< Set FSL to STANDBY state
};

/// Error codes for ctrl/status protocol responses
/// Error codes for ctrl/status protocol responses
enum FslCtrlErrorCode : uint8_t
{
    FSL_CTRL_ERR_NONE = 0,           ///< No error
    FSL_CTRL_ERR_UNKNOWN_OPCODE = 1, ///< Unknown/unsupported opcode
    FSL_CTRL_ERR_INVALID_PARAM = 2,  ///< Invalid parameter
    FSL_CTRL_ERR_INTERNAL = 3,       ///< Internal error
    FSL_CTRL_ERR_QUEUE_FULL = 4,     ///< Ctrl message queue is full (buffer overflow)
};

/// Error codes for data-link protocol responses
enum FslDataLinkErrorCode : uint8_t
{
    FSL_DATA_LINK_ERR_NONE = 0,            ///< No error
    FSL_DATA_LINK_ERR_UNKNOWN_OPCODE = 1,  ///< Unknown/unsupported opcode
    FSL_DATA_LINK_ERR_INVALID_PARAM = 2,   ///< Invalid parameter
    FSL_DATA_LINK_ERR_INTERNAL = 3,        ///< Internal error
    FSL_DATA_LINK_ERR_UDS_WOULD_BLOCK = 4, ///< UDS buffer would block
};

/// Header for all ctrl/status protocol messages
typedef struct FslCtrlHeader
{
    FslCtrlOpcode ctrl_opcode;        ///< Control opcode
    FslCtrlErrorCode ctrl_error_code; ///< Error code (for responses)
    uint16_t ctrl_length;             ///< Payload length (bytes)
    uint32_t ctrl_seq_id;             ///< Sequence ID (for matching requests/responses)
} FslCtrlHeader;

/// Response to GET_CBIT (status query)
typedef struct FslCtrlGetCbitResponse
{
    FslCtrlHeader header;
    FslStates fsl_state; ///< Current FSL state
} FslCtrlGetCbitResponse;

/// Request to set FSL to OPER state
typedef struct FslCtrlSetOperRequest
{
    FslCtrlHeader header;
} FslCtrlSetOperRequest;

/// Request to set FSL to STANDBY state
typedef struct FslCtrlSetStandbyRequest
{
    FslCtrlHeader header;
} FslCtrlSetStandbyRequest;

/// General ACK response for ctrl requests
typedef struct FslCtrlGeneralAckResponse
{
    FslCtrlHeader header;
} FslCtrlGeneralAckResponse;