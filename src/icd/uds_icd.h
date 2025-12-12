#ifndef UDS_ICD_H
#define UDS_ICD_H

// UDS (Unix Domain Socket) paths and buffer sizes for FSL communication
// This file is the single source of truth for all UDS endpoints used by FSL and its client applications.
// Do not edit config.xml for UDS definitions; use this header instead.

// Downlink (FSL is server)
#define UDS_DL_EL_H "/tmp/DL_EL_H"
#define UDS_DL_EL_L "/tmp/DL_EL_L"
#define UDS_DL_PLMG_H "/tmp/DL_PLMG_H"
#define UDS_DL_PLMG_L "/tmp/DL_PLMG_L"
#define UDS_DL_FSW_HIGH "/tmp/FSW_HIGH_DL"
#define UDS_DL_FSW_LOW "/tmp/FSW_LOW_DL"
#define UDS_DL_BUFFER_SIZE 1024

// Uplink (FSL is client)
#define UDS_UL_FSW "/tmp/FSW_UL"
#define UDS_UL_PLMG "/tmp/UL_PLMG"
#define UDS_UL_EL "/tmp/UL_EL"

// Uplink mapping opcodes
#define UDS_UL_OPCODE_FSW 1
#define UDS_UL_OPCODE_PLMG 2
#define UDS_UL_OPCODE_EL 3

// Ctrl/Status UDS channels
#define UDS_CTRL_FSW_REQ "/tmp/fsw_to_fcom"
#define UDS_CTRL_FSW_RESP "/tmp/fcom_to_fsw"
#define UDS_CTRL_FSW_REQ_SIZE 1024

#define UDS_CTRL_DYNAMIC_REQ "/tmp/fsw_dynamic"
#define UDS_CTRL_DYNAMIC_REQ_SIZE 1024

#define UDS_CTRL_PLMG_REQ "/tmp/plmg_to_fcom"
#define UDS_CTRL_PLMG_RESP "/tmp/fcom_to_plmg"
#define UDS_CTRL_PLMG_REQ_SIZE 1024

#define UDS_CTRL_TELEMETRY_REQ "/tmp/plmg_telemetry"

#define UDS_CTRL_EL_REQ "/tmp/el_to_fcom"
#define UDS_CTRL_EL_RESP "/tmp/fcom_to_el"
#define UDS_CTRL_EL_REQ_SIZE 1024

#endif // UDS_ICD_H
