#pragma once

#include <cstdint>
#include <cstddef>

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
