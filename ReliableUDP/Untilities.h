#pragma once

bool LoadFile(const char* filename, unsigned char** data, size_t* size);

bool SaveFile(const char* filename, const unsigned char* data, size_t size);

bool SendFile(const unsigned char* data);

bool ReceiveFile(unsigned char** data);

bool VerifyFile(const unsigned char* data, unsigned int hash);