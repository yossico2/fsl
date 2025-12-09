#pragma once

#include <cstdint>
#include <cstddef>

typedef struct FslGslHeader
{
    uint16_t msg_opcode;
    uint16_t msg_length;
    uint32_t msg_id;
} FslGslHeader;

static const size_t FSL_GSL_HEADER_SIZE = sizeof(FslGslHeader);
