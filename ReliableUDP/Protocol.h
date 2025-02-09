/*
* FILE : Protocol.c
* PROJECT : SENG2040 - ASSIGNMENT 1
* PROGRAMMER :  Lee Yu-Hsuan, 8954099
* FIRST VERSION : 2025-02-04
* DESCRIPTION :
*   This header file defines the data transmission protocol for file slicing.
*   It includes the `PacketMeta` structure, which stores file metadata such as
*   filename, size, total slices, and MD5 hash for integrity verification.
*   Additionally, it defines `PacketSlice`, which represents individual data
*   packets used for segmented file transmission. These structures ensure that
*   files can be reliably split, transmitted, and reconstructed.
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
* 
*     PacketMeta Segment:
* 
* 	7             1     0
* +-------------------------+    0
* |typeFlag (1B): |data|meta|
* +-------------------------+    1
* |	      filename (200B)   |
* +-------------------------+  201
* |       fileSize (8B)     |
* +-------------------------+  209
* |    totalSlices (8B)     |
* +-------------------------+  217
* |            md5 (16B)    |
* +-------------------------+  233
* |        padding          |
* +-------------------------+  256
* 
* 
*      PacketSlice Segment:
* 
* 	7             1     0
* +-------------------------+    0
* |typeFlag (1B): |data|meta|
* +-------------------------+    1
* |          id (8B)        |
* +-------------------------+    9
* |        data (247B)      |
* +-------------------------+  256
* 
*/

#include <cstdint>

#define PACKET_SIZE         256
#define MAX_FILENAME_LENGTH 200
#define MD5_HASH_LENGTH     16

#define PADDING_SIZE        (PACKET_SIZE - 1 - MAX_FILENAME_LENGTH - 8 * 2 - MD5_HASH_LENGTH)
#define DATA_SIZE           (PACKET_SIZE - 1 - 8)

enum PacketType : uint8_t {
    TYPE_META = 0x01, // 0000 0001
    TYPE_DATA = 0x02  // 0000 0010
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

#endif