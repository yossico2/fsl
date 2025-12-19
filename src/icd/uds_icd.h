#ifndef UDS_ICD_H
#define UDS_ICD_H

// UDS (Unix Domain Socket) paths and buffer sizes for FSL communication
// This file is the single source of truth for all UDS endpoints used by FSL and its client applications.
// Do not edit config.xml for UDS definitions; use this header instead.

// Downlink (FSL is server)
#define UDS_DL_EL_H "/tmp/DL_EL_H"         // EL high downlink
#define UDS_DL_EL_L "/tmp/DL_EL_L"         // EL low downlink
#define UDS_DL_PLMG_H "/tmp/DL_PLMG_H"     // PLMG high downlink
#define UDS_DL_PLMG_L "/tmp/DL_PLMG_L"     // PLMG low downlink
#define UDS_DL_FSW_HIGH "/tmp/FSW_HIGH_DL" // FSW high downlink
#define UDS_DL_FSW_LOW "/tmp/FSW_LOW_DL"   // FSW low downlink
#define UDS_DL_BUFFER_SIZE 1024            // Downlink buffer size

// Uplink (FSL is client)
#define UDS_UL_FSW "/tmp/FSW_UL"   // FSW uplink
#define UDS_UL_PLMG "/tmp/UL_PLMG" // PLMG uplink
#define UDS_UL_EL "/tmp/UL_EL"     // EL uplink

// Uplink mapping opcodes
#define UDS_UL_OPCODE_FSW 1  // FSW opcode
#define UDS_UL_OPCODE_PLMG 2 // PLMG opcode
#define UDS_UL_OPCODE_EL 3   // EL opcode

// Ctrl/Status UDS channels
#define UDS_CTRL_FSW_REQ "/tmp/fsw_to_fcom"  // FSW request
#define UDS_CTRL_FSW_RESP "/tmp/fcom_to_fsw" // FSW response
#define UDS_CTRL_FSW_REQ_SIZE 1024           // FSW request buffer size

// Dynamic ctrl channel
#define UDS_CTRL_DYNAMIC_REQ "/tmp/fsw_dynamic" // Dynamic request
#define UDS_CTRL_DYNAMIC_REQ_SIZE 1024          // Dynamic request buffer size

// PLMG ctrl channel
#define UDS_CTRL_PLMG_REQ "/tmp/plmg_to_fcom"  // PLMG request
#define UDS_CTRL_PLMG_RESP "/tmp/fcom_to_plmg" // PLMG response
#define UDS_CTRL_PLMG_REQ_SIZE 1024            // PLMG request buffer size

// Telemetry ctrl channel
#define UDS_CTRL_TELEMETRY_REQ "/tmp/plmg_telemetry" // Telemetry request

// EL ctrl channel
#define UDS_CTRL_EL_REQ "/tmp/el_to_fcom"  // EL request
#define UDS_CTRL_EL_RESP "/tmp/fcom_to_el" // EL response
#define UDS_CTRL_EL_REQ_SIZE 1024          // EL request buffer size

#endif // UDS_ICD_H
