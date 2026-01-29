#include "WAVFileInfo.h"
#include "EOUtils.h"
#include <cstring> // For memset()
using std::string;
using std::fstream;
using std::ifstream;


namespace EOUtils
{
	WAVFileInfo::WAVFileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond, int16_t pByteRate, int16_t pBitsPerSample)
		: AudioFileInfo(pNumChannels, pSampleRateHz, pBytesPerSecond, pByteRate, pBitsPerSample),
		  mSubchunk2Size(0),
		  mDataSizeBytes(0)
	{
		memset((void*)mWAVHeader, 0, sizeof(mWAVHeader));
		memset((void*)mRIFFType, 0, sizeof(mRIFFType));
		memset((void*)mSubchunk2ID, 0, sizeof(mSubchunk2ID));
	}

	WAVFileInfo::WAVFileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo)
	{ }

	AudioFileResultType WAVFileInfo::read(fstream& pFStream)
	{
		AudioFileResultType result;

		if (!pFStream.is_open())
		{
			result.addError("WAVFileInfo::read(): The file is not open");
			return result;
		}

		// WAV file format:
		// http://www.ringthis.com/dev/wave_format.htm
		// http://soundfile.sapp.org/doc/WaveFormat/
		// In order for this to work, the fstream must have been opened with 'in' and 'binary'.
		// Also, this assumes that the stream is at the beginning of the file.

		// RIFF chunk (12 bytes total)
		// Read the header (first 4 bytes)
		pFStream.read(mWAVHeader, sizeof(mWAVHeader));
		// Read the file size (4 bytes)
		pFStream.read((char*)&mFileSize, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mFileSize, sizeof(mFileSize));
		// Read the RIFF type
		pFStream.read(mRIFFType, sizeof(mRIFFType));

		// Format chunk (24 bytes total)
		char buffer[4];
		// "fmt " (ASCII characters)
		pFStream.read(buffer, 4);
		// Length of format chunk (always 16)
		pFStream.read(buffer, 4);
		// 2 bytes (value always 1)
		pFStream.read(buffer, 2);
		// # of channels (2 bytes)
		pFStream.read((char*)&mNumChannels, 2);
		if (machineIsBigEndian)
			reverseBytes((void*)&mNumChannels, sizeof(mNumChannels));
		// Sample rate (4 bytes)
		pFStream.read((char*)&mSampleRateHz, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mSampleRateHz, sizeof(mSampleRateHz));
		// Bytes per second (4 bytes)
		pFStream.read((char*)&mBytesPerSecond, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mBytesPerSecond, sizeof(mBytesPerSecond));
		// Byte rate (2 bytes)
		pFStream.read((char*)&mByteRate, 2);
		if (machineIsBigEndian)
			reverseBytes((void*)&mByteRate, sizeof(mByteRate));
		// Bits per sample (2 bytes)
		pFStream.read((char*)&mBitsPerSample, 2);
		if (machineIsBigEndian)
			reverseBytes((void*)&mBitsPerSample, sizeof(mBitsPerSample));
		/*
		// Subchunk 2 ID & subchunk 2 size (4 bytes each)
		pFStream.read(mSubchunk2ID, sizeof(mSubchunk2ID));
		pFStream.read((char*)&mSubchunk2Size, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mSubchunk2Size, sizeof(mSubchunk2Size));
		*/

		// Data chunk
		// "data" (ASCII characters)
		pFStream.read(buffer, 4);
		// Length of data to follow (4 bytes)
		pFStream.read((char*)&mDataSizeBytes, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mDataSizeBytes, sizeof(mDataSizeBytes));

		// Total of 52 bytes read up to this point.
		// Total of 44 bytes read up to this point.

		return result;
	}

	AudioFileResultType WAVFileInfo::write(std::fstream& pFStream)
	{
		AudioFileResultType result;
		if (!pFStream.is_open())
		{
			result.addError("WAVFileInfo::write(): The file is not open");
			return result;
		}

		// RIFF chunk (12 bytes total)
		// Header (first 4 bytes)
		pFStream.write((const char*)mWAVHeader, sizeof(mWAVHeader));
		// File size (4 bytes)
		int32_t data_4_bytes = mFileSize;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// RIFF type (4 bytes)
		pFStream.write((const char*)mRIFFType, sizeof(mRIFFType));

		// Format chunk (24 bytes total)
		// "fmt " (ASCII characters)
		pFStream.write("fmt ", 4);
		// Length of format chunk (always 16)
		data_4_bytes = 16;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// 2 bytes (value always 1)
		int16_t data_2_bytes = 1;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		// # of channels (2 bytes)
		data_2_bytes = mNumChannels;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		// Sample rate (4 bytes)
		data_4_bytes = mSampleRateHz;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// Bytes per second (4 bytes)
		data_4_bytes = mBytesPerSecond;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// Byte rate (2 bytes)
		data_2_bytes = mByteRate;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		// Bits per sample (2 bytes)
		data_2_bytes = mBitsPerSample;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		/*
		// Subchunk 2 ID & subchunk 2 size (4 bytes each)
		pFStream.write("fact", 4);
		data_4_bytes = 4;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		*/

		// Data chunk
		// "data" (ASCII characters)
		pFStream.write("data", 4);
		// Length of data to follow (4 bytes)
		data_4_bytes = mDataSizeBytes;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));

		// Total of 52 bytes written
		// Total of 44 bytes written

		return result;
	}

	AudioFileResultType WAVFileInfo::updateFileSizeSizeInFile(std::fstream& pOutFStream, int32_t pFileSize)
	{
		AudioFileResultType result;
		if (!pOutFStream.is_open())
		{
			result.addError("WAVFileInfo::updateFileSizeInFile(): pOutFStream is not open");
			return result;
		}

		FileSize(pFileSize);
		pOutFStream.seekg(4, std::ios_base::beg);
		int32_t data_4_bytes = pFileSize;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pOutFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		return result;
	}

	AudioFileResultType WAVFileInfo::updateDataSizeSizeInFile(std::fstream& pOutFStream, int32_t pDataSize)
	{
		AudioFileResultType result;
		if (!pOutFStream.is_open())
		{
			result.addError("WAVFileInfo::updateDataSizeSizeInFile(): pOutFStream is not open");
			return result;
		}

		// (should be at 40 bytes in the file)
		//pOutFStream.seekg(48, std::ios_base::beg);
		pOutFStream.seekg(40, std::ios_base::beg);
		int32_t data_4_bytes = pDataSize;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pOutFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// Lead.wav is 11,113,256 bytes and the data size is 11,113,200
		return result;
	}

	string WAVFileInfo::WAVHeader() const
	{
		string header;
		header.reserve(sizeof(mWAVHeader));
		for (char ch : mWAVHeader)
			header += ch;
		return header;
	}

	string WAVFileInfo::RIFFType() const
	{
		string rifftype;
		rifftype.reserve(sizeof(mRIFFType));
		for (char ch : mRIFFType)
			rifftype += ch;
		return rifftype;
	}

	string WAVFileInfo::Subchunk2ID() const
	{
		string subchunk2ID;
		subchunk2ID.reserve(sizeof(mSubchunk2ID));
		for (char ch : mSubchunk2ID)
			subchunk2ID += ch;
		return subchunk2ID;
	}

	int32_t WAVFileInfo::Subchunk2Size() const
	{
		return mSubchunk2Size;
	}

	int32_t WAVFileInfo::DataSizeBytes() const
	{
		return mDataSizeBytes;
	}

	int16_t WAVFileInfo::BitsPerSample() const
	{
		return AudioFileInfo::BitsPerSample();
	}

	void WAVFileInfo::BitsPerSample(int16_t pBitsPerSample)
	{
		AudioFileInfo::BitsPerSample(pBitsPerSample);
	}

	void WAVFileInfo::SetWAVHeaderAndRIFFType()
	{
		mWAVHeader[0] = 'R';
		mWAVHeader[1] = 'I';
		mWAVHeader[2] = 'F';
		mWAVHeader[3] = 'F';
		mRIFFType[0] = 'W';
		mRIFFType[1] = 'A';
		mRIFFType[2] = 'V';
		mRIFFType[3] = 'E';
	}

	void WAVFileInfo::SetSubchunk2IDAndSize()
	{
		mSubchunk2ID[0] = 'f';
		mSubchunk2ID[1] = 'a';
		mSubchunk2ID[2] = 'c';
		mSubchunk2ID[3] = 't';
		mSubchunk2Size = 4;
	}

	/////////////////////////////
	// Static methods

	size_t WAVFileInfo::WAVFileHdrSize()
	{
		//return 52; // 52 bytes
		return 44; // 44 bytes
	}

	int16_t WAVFileInfo::BitsPerSample(const char* pFilename)
	{
		int16_t bitsPerSample = 0;

		size_t fileSize = getFileSize(pFilename);
		if (fileSize >= WAVFileHdrSize())
		{
			ifstream inFile(pFilename, std::ios::in | std::ios::binary);
			if (inFile.is_open())
			{
				inFile.seekg(34, std::ios_base::beg);
				inFile.read((char*)&bitsPerSample, 2);
				inFile.close();
				if (machineIsBigEndian)
					reverseBytes((void*)&bitsPerSample, sizeof(bitsPerSample));
			}
		}

		return bitsPerSample;
	}

	bool WAVFileInfo::isWAVFile(const char* pFilename)
	{
		bool fileIsWAVFile = false;

		// Open the file and check its WAV header and RIFF header
		size_t fileSize = getFileSize(pFilename);
		if (fileSize >= WAVFileHdrSize())
		{
			ifstream inFile(pFilename, std::ios::in | std::ios::binary);
			if (inFile.is_open())
			{
				char WAVHeader[4]; // The WAV header (4 bytes, "RIFF")
				char RIFFType[4];  // The RIFF type (4 bytes, "WAVE")

				// Read the header (first 4 bytes)
				inFile.read(WAVHeader, sizeof(WAVHeader));
				// Skip 4 bytes (file size)
				inFile.seekg(4, std::ios_base::cur);
				// Read the RIFF type
				inFile.read(RIFFType, sizeof(RIFFType));
				inFile.close();

				bool hdrIsRIFF = (WAVHeader[0] == 'R' && WAVHeader[1] == 'I' && WAVHeader[2] == 'F' && WAVHeader[3] == 'F');
				bool RIFFTypeIsWAVE = (RIFFType[0] == 'W' && RIFFType[1] == 'A' && RIFFType[2] == 'V' && RIFFType[3] == 'E');
				fileIsWAVFile = hdrIsRIFF && RIFFTypeIsWAVE;
			}
		}

		return fileIsWAVFile;
	}
}
