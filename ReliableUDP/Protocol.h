#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

#define MAX_FILENAME_LENGTH 256

enum PacketType {
    FILE_INFO,
    FILE_DATA,
    ACK,
    MD5_CHECK,
    ERROR
};

struct PacketHeader {
    uint32_t sequence;
    uint16_t length;
    uint16_t flags;
};

#endif