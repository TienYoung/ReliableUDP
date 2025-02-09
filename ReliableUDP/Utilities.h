/*
* FILE : Utilities.h
* PROJECT : SENG2040 - ASSIGNMENT 1
* PROGRAMMER : Tian Yang, 8952896
* FIRST VERSION : 2025-02-06
* DESCRIPTION :
*   This file provides a `FileSlices` class, which facilitates file slicing,
*   metadata handling, verification, and reconstruction. It enables breaking
*   a file into smaller packets, computing MD5 hashes for integrity checking,
*   and reassembling the file from its slices.
*/

#pragma once

#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <chrono>

#include "Protocol.h"
#include "md5.h"

/*
 * Class : FileSlices
 * Description :
 *   This class provides functionality to load, verify, save, and manage file slices.
 *   It supports splitting a file into smaller slices, computing MD5 hash for verification,
 *   and reconstructing the file from its slices.
 */
class FileSlices
{
public:
	/*
	 * Function : Load
	 * Description :
	 *   Loads a file and splits it into slices for transmission or storage.
	 *   Computes MD5 checksum for integrity verification.
	 * Parameters :
	 *   const char* filename - The name of the file to load and slice.
	 * Return :
	 *   bool - Returns true if the file is successfully loaded, false otherwise.
	 */
	bool Load(const char* filename)
	{
		assert(filename != nullptr);
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

	/*
	 * Function : Verify
	 * Description :
	 *   Verifies the integrity of the file slices by computing the MD5 hash
	 *   and comparing it with the stored hash in metadata.
	 * Parameters :
	 *   None
	 * Return :
	 *   bool - Returns true if the computed MD5 hash matches the stored hash, false otherwise.
	 */
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

	/*
	 * Function : Save
	 * Description :
	 *   Reconstructs the original file from the stored slices and saves it.
	 * Parameters :
	 *   const char* filename - (Optional) The name of the output file. If NULL, the original filename is used.
	 * Return :
	 *   bool - Returns true if the file is successfully saved, false otherwise.
	 */
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

			// A1: Writing the pieces out to disk
			file.write(m_slices[i].data, size);
		}
		file.close();

		return true;
	}

	/*
	 * Function : Reset
	 * Description :
	 *   Clears all stored slices and metadata, resetting the object to its initial state.
	 * Parameters :
	 *   None
	 * Return :
	 *   void
	 */
	void Reset()
	{
		m_ready = false;
		m_meta = { 0 };
		m_slices.clear();
	}

	/*
	 * Function : IsReady
	 * Description :
	 *   Checks if all slices of the file have been received and are ready for reconstruction.
	 * Parameters :
	 *   None
	 * Return :
	 *   bool - Returns true if the file is ready for reconstruction, false otherwise.
	 */
	bool IsReady() const
	{
		return m_ready;
	}

	/*
	 * Function : GetMeta
	 * Description :
	 *   Retrieves the metadata of the loaded file, including filename, size, total slices, and MD5 hash.
	 * Parameters :
	 *   None
	 * Return :
	 *   const PacketMeta* - A pointer to the file metadata structure.
	 */
	const PacketMeta* GetMeta() const 
	{
		return &m_meta;
	}

	/*
	 * Function : GetTotal
	 * Description :
	 *   Returns the total number of slices stored.
	 * Parameters :
	 *   None
	 * Return :
	 *   size_t - The total number of file slices.
	 */
	size_t GetTotal() const
	{
		return m_slices.size();
	}

	/*
	 * Function : GetSlice
	 * Description :
	 *   Retrieves a specific slice of the file by its ID.
	 * Parameters :
	 *   size_t id - The index of the slice to retrieve.
	 * Return :
	 *   const PacketSlice* - A pointer to the requested slice, or NULL if the ID is out of range.
	 */
	const PacketSlice* GetSlice(size_t id) const
	{
		if (id >= m_slices.size())
		{
			std::cerr << "Slice ID is out of the boundary." << std::endl;
			return nullptr;
		}

		return &m_slices[id];
	}

	/*
	 * Function : Deserialize
	 * Description :
	 *   Processes incoming data packets and reconstructs file slices.
	 *   Determines if the packet is metadata or a data slice and stores it accordingly.
	 * Parameters :
	 *   const unsigned char* data - A pointer to the received packet data.
	 * Return :
	 *   bool - Returns true if the packet is successfully processed, false otherwise.
	 */
	bool Deserialize(const unsigned char* data)
	{
		PacketType typeFlag = static_cast<PacketType>(data[0]);
		// A1: Receiving the file metadata
		if (typeFlag == TYPE_META)
		{
			const PacketMeta* meta = reinterpret_cast<const PacketMeta*>(data);
			m_meta.typeFlag = typeFlag;
			strcpy_s(m_meta.filename, MAX_FILENAME_LENGTH, meta->filename);
			m_meta.fileSize = meta->fileSize;
			m_meta.totalSlices = meta->totalSlices;
			memcpy(m_meta.md5, meta->md5, MD5_HASH_LENGTH);

			m_slices.resize(m_meta.totalSlices);

			return true;
		}
		// A1: Receiving the file pieces
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

			return true;
		}

		return false;
	}

private:
	bool m_ready = false;
	PacketMeta m_meta = { 0 };
	std::vector<PacketSlice> m_slices;
};