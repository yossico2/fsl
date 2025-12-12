#pragma once

#include <cstdint>
#include <cstddef>

/// GSL-FSL protocol message header (UDP framing)
typedef struct GslFslHeader
{
    uint16_t opcode; ///< Message opcode (application-specific)
    uint16_t length; ///< Payload length (bytes)
    uint32_t seq_id; ///< Unique message ID (monotonic)
} GslFslHeader;

/// Size of GslFslHeader struct (for framing)
static const size_t FSL_GSL_HEADER_SIZE = sizeof(GslFslHeader);

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

typedef struct FslDataLinkErrorResponse
{
    uint16_t opcode;                 ///< Original message opcode
    FslDataLinkErrorCode error_code; ///< Error code
} FslDataLinkErrorResponse;

////////////////////////////////////////////////////////////////////////
// FCOM-FSW
////////////////////////////////////////////////////////////////////////

/// FCOM-FSW opcodes
typedef enum FCOM_FSW_OPCODE_EN : uint8_t
{
    FCOM_FSW_OP_NOP = 0,
    FCOM_FSW_OP_SET_STATE_STANDBY = 3,
    FCOM_FSW_OP_SET_STATE_OPER = 5,
} FCOM_FSW_OPCODE_EN;

/// FCOM-FSW error codes
typedef enum FCOM_FSW_ERROR_EN : uint8_t
{
    FCOM_FSW_E_UNDEFINED = 0,
    FCOM_FSW_E_ACK = 1,
    FCOM_FSW_E_GENERAL = 6,
} FCOM_FSW_ERROR_EN;

/// FCOM-FSW command/status header
typedef struct fcom_fsw_CS_Header
{
    FCOM_FSW_OPCODE_EN opcode;
    FCOM_FSW_ERROR_EN error;
    uint16_t seq_id;
    uint16_t length; /// payload length
} fcom_fsw_CS_Header;

////////////////////////////////////////////////////////////////////////
// FCOM-PLMG
////////////////////////////////////////////////////////////////////////

/// FCOM-PLMG opcodes
typedef enum FCOM_PLMG_OPCODE_EN : uint8_t
{
    FCOM_PLMG_OP_NOP = 0,
} FCOM_PLMG_OPCODE_EN;

/// FCOM-PLMG error codes
typedef enum FCOM_PLMG_ERROR_EN : uint8_t
{
    FCOM_PLMG_E_UNDEFINED = 0,
    FCOM_PLMG_E_ACK = 1,
    FCOM_PLMG_E_GENERAL = 5,
} FCOM_PLMG_ERROR_EN;

/// FCOM-PLMG general header
typedef struct plmg_fcom_Header
{
    FCOM_PLMG_OPCODE_EN opcode;
    FCOM_PLMG_ERROR_EN error;
    uint16_t seq_id;
    uint16_t length; /// payload length
} plmg_fcom_Header;