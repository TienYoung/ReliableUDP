#pragma once

#include <vector>
#include <fstream>
#include <iostream>

#define PACK_SIZE 256
#define DATA_SIZE 244 // sizeof(packet) - 3 * sizof(int)

struct FileSlice
{
	int id;
	int number;
	int size;
	char data[DATA_SIZE];
};

std::vector<FileSlice> LoadFileIntoSlices(const char* filename)
{
	std::fstream file(filename, std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Error Opening: " << filename << std::endl;
	}

	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	int count = (fileSize + DATA_SIZE - 1) / DATA_SIZE; // Round up
	std::vector<FileSlice> slices(count);
	for (size_t i = 0; i < count; i++)
	{
		slices[i].id = i;
		slices[i].number = count;

		slices[i].size = DATA_SIZE;
		if (i == count - 1)
		{
			slices[i].size = fileSize - DATA_SIZE * (count - 1);
		}

		file.read(slices[i].data, slices[i].size);
	}
	
	file.close();

	return slices;
}

bool SaveFile(const char* filename, const unsigned char* data, size_t size);

bool SendFile(const unsigned char* data);

bool ReceiveFile(unsigned char** data);

bool VerifyFile(const unsigned char* data, unsigned int hash);