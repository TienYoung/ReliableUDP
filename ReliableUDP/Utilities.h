#pragma once

#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>

#include "Protocol.h"
#include "md5.h"

class FileSlices
{
public:
	bool Load(const char* filename)
	{
		if (!std::filesystem::exists(filename))
		{
			std::cerr << "Error: File Not exists! " << filename << std::endl;
			return false;
		}

		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file)
		{
			std::cerr << "Error: Failed opening file to read! " << filename << std::endl;
			return false;
		}

		m_meta.typeFlag = TYPE_META;
		strcpy_s(m_meta.filename, MAX_FILENAME_LENGTH, filename);
		m_meta.fileSize = file.tellg();

		FILE* source = nullptr;
		fopen_s(&source, filename, "rb");
		md5File(source, m_meta.md5);
		fclose(source);

		file.seekg(0, std::ios::beg);

		m_meta.totalSlices = (m_meta.fileSize + DATA_SIZE - 1) / DATA_SIZE; // Round up
		m_slices.resize(m_meta.totalSlices);
		for (size_t i = 0; i < m_meta.totalSlices; i++)
		{
			m_slices[i].typeFlag = TYPE_DATA;
			m_slices[i].id = i;

			file.read(m_slices[i].data, DATA_SIZE);
		}
		file.close();

		return true;
	}

	bool Verify()
	{
		MD5Context ctx;
		md5Init(&ctx);

		for (size_t i = 0; i < m_slices.size(); i++)
		{
			size_t size = DATA_SIZE;
			if (i == m_meta.totalSlices - 1)
			{
				size = m_meta.fileSize - DATA_SIZE * (m_meta.totalSlices - 1);
			}
			md5Update(&ctx, reinterpret_cast<uint8_t*>(m_slices[i].data), size);
		}
		md5Finalize(&ctx);

		if (memcmp(m_meta.md5, ctx.digest, MD5_HASH_LENGTH) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Save(const char* filename = nullptr) const
	{
		if (filename == nullptr)
		{
			filename = m_meta.filename;
		}

		std::ofstream file(filename, std::ios::binary);
		if (!file)
		{
			std::cerr << "Error: Failed opening file to write! " << filename << std::endl;
			return false;
		}
		for (size_t i = 0; i < m_slices.size(); i++)
		{
			size_t size = DATA_SIZE;
			if (i == m_meta.totalSlices - 1)
			{
				size = m_meta.fileSize - DATA_SIZE * (m_meta.totalSlices - 1);
			}

			file.write(m_slices[i].data, size);
		}
		file.close();

		return true;
	}

	bool IsRead() const
	{
		return m_ready;
	}

	const PacketMeta* GetMeta() const 
	{
		return &m_meta;
	}

	size_t GetTotal() const
	{
		return m_slices.size();
	}

	const PacketSlice* GetSlice(size_t id) const
	{
		if (id >= m_slices.size())
		{
			std::cerr << "Slice ID is out of the boundary." << std::endl;
			return nullptr;
		}

		return &m_slices[id];
	}

	void Deserialize(const unsigned char* data)
	{
		PacketType typeFlag = static_cast<PacketType>(data[0]);
		if (typeFlag == TYPE_META)
		{
			const PacketMeta* meta = reinterpret_cast<const PacketMeta*>(data);
			m_meta.typeFlag = typeFlag;
			strcpy_s(m_meta.filename, MAX_FILENAME_LENGTH, meta->filename);
			m_meta.fileSize = meta->fileSize;
			m_meta.totalSlices = meta->totalSlices;
			memcpy(m_meta.md5, meta->md5, MD5_HASH_LENGTH);

			m_slices.resize(m_meta.totalSlices);
		}
		else if (typeFlag == TYPE_DATA)
		{
			const PacketSlice* slice = reinterpret_cast<const PacketSlice*>(data);
			m_slices[slice->id].typeFlag = typeFlag;
			m_slices[slice->id].id = slice->id;
			memcpy(m_slices[slice->id].data, slice->data, DATA_SIZE);
			
			// TODO: replace with list
			if (slice->id == m_meta.totalSlices - 1)
			{
				m_ready = true;
			}
		}
	}

private:
	bool m_ready;
	PacketMeta m_meta;
	std::vector<PacketSlice> m_slices;
};

//
//bool SendFile(const unsigned char* data);
//
//bool ReceiveFile(unsigned char** data);
//