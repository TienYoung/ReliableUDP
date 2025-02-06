#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

#define MAX_FILENAME_LENGTH 256

enum PacketType {
    FILE_INFO,
    FILE_DATA,
    ACK,
    MD5_CHECK,
    ERROR_PACKET
};

struct PacketHeader {
    uint32_t sequence;
    uint16_t length;
    uint16_t flags;
};

struct FileMetadata {
    char filename[MAX_FILENAME_LENGTH];
    uint32_t filesize;
};

#endif