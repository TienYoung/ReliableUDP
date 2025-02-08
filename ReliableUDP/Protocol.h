#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

#define PACKET_SIZE         256
#define MAX_FILENAME_LENGTH 200
#define MD5_HASH_LENGTH     16

#define PADDING_SIZE        (PACKET_SIZE - 1 - MAX_FILENAME_LENGTH - 8 * 2 - MD5_HASH_LENGTH)
#define DATA_SIZE           (PACKET_SIZE - 1 - 8)

enum PacketType : uint8_t {
    TYPE_META = 0x01, // 0000 0001
    TYPE_DATA = 0x02  // 0000 0010
    //ACK
    //MD5_CHECK,
    //ERROR
};

// Make sure all packets are fixed size(256) and 1 byte aligned.
#pragma pack(push, 1)
struct PacketMeta
{
    uint8_t     typeFlag;
    char        filename[MAX_FILENAME_LENGTH];
    uint64_t    fileSize;
    uint64_t    totalSlices;
    uint8_t     md5[MD5_HASH_LENGTH];
    uint8_t     padding[PADDING_SIZE];
};

struct PacketSlice
{
    uint8_t     typeFlag;
	uint64_t    id;
	char        data[DATA_SIZE];
};
#pragma pack(pop)

struct FileMetadata {
    char filename[MAX_FILENAME_LENGTH];
    uint32_t filesize;
};

#endif