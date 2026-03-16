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
		  mDataSizeBytes(0),
		  mAudioDataOffset(44)
	{
		memset((void*)mWAVHeader, 0, sizeof(mWAVHeader));
		memset((void*)mRIFFType, 0, sizeof(mRIFFType));
		memset((void*)mSubchunk2ID, 0, sizeof(mSubchunk2ID));
	}

	WAVFileInfo::WAVFileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo),
		  mSubchunk2Size(0),
		  mDataSizeBytes(0),
		  mAudioDataOffset(44)
	{
		memset((void*)mWAVHeader, 0, sizeof(mWAVHeader));
		memset((void*)mRIFFType, 0, sizeof(mRIFFType));
		memset((void*)mSubchunk2ID, 0, sizeof(mSubchunk2ID));
	}

	AudioFileResultType WAVFileInfo::read(fstream& pFStream)
	{
		AudioFileResultType result;

		if (!pFStream.is_open())
		{
			result.addError("WAVFileInfo::read(): The file is not open");
			return result;
		}

		// WAV file format:
		// http://soundfile.sapp.org/doc/WaveFormat/
		// The stream must be opened with 'in' and 'binary'.
		// Seek to the beginning so we always parse from the start.
		pFStream.seekg(0, std::ios_base::beg);

		// ── RIFF descriptor (12 bytes) ──────────────────────────────────────────
		// "RIFF" header (4 bytes)
		pFStream.read(mWAVHeader, sizeof(mWAVHeader));
		if (mWAVHeader[0] != 'R' || mWAVHeader[1] != 'I' ||
		    mWAVHeader[2] != 'F' || mWAVHeader[3] != 'F')
		{
			result.addError("WAVFileInfo::read(): Not a RIFF file");
			return result;
		}

		// File size (4 bytes)
		pFStream.read((char*)&mFileSize, 4);
		if (machineIsBigEndian)
			reverseBytes((void*)&mFileSize, sizeof(mFileSize));

		// RIFF type (4 bytes) — must be "WAVE"
		pFStream.read(mRIFFType, sizeof(mRIFFType));
		if (mRIFFType[0] != 'W' || mRIFFType[1] != 'A' ||
		    mRIFFType[2] != 'V' || mRIFFType[3] != 'E')
		{
			result.addError("WAVFileInfo::read(): RIFF type is not WAVE");
			return result;
		}

		// ── Chunk scan ──────────────────────────────────────────────────────────
		// Scan through all chunks until both "fmt " and "data" are located.
		// This correctly handles WAV files with extra chunks (e.g. LIST, fact,
		// bext) between the format descriptor and the audio data.
		bool foundFmt  = false;
		bool foundData = false;

		char   chunkID[4];
		int32_t chunkSize = 0;

		while (!pFStream.eof() && (!foundFmt || !foundData))
		{
			pFStream.read(chunkID, 4);
			if (pFStream.gcount() < 4)
				break;

			pFStream.read((char*)&chunkSize, 4);
			if (pFStream.gcount() < 4)
				break;
			if (machineIsBigEndian)
				reverseBytes((void*)&chunkSize, sizeof(chunkSize));

			if (chunkID[0] == 'f' && chunkID[1] == 'm' &&
			    chunkID[2] == 't' && chunkID[3] == ' ')
			{
				// ── fmt chunk ───────────────────────────────────────────────────
				// Minimum size is 16 bytes (PCM); extensible format uses 18 or 40.
				if (chunkSize < 16)
				{
					result.addError("WAVFileInfo::read(): fmt chunk is too small");
					return result;
				}

				// Audio format (2 bytes): 1 = PCM, 3 = IEEE float, 0xFFFE = extensible
				int16_t audioFormat = 0;
				pFStream.read((char*)&audioFormat, 2);
				if (machineIsBigEndian)
					reverseBytes((void*)&audioFormat, sizeof(audioFormat));

				// Number of channels (2 bytes)
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

				// Block align / byte rate (2 bytes)
				pFStream.read((char*)&mByteRate, 2);
				if (machineIsBigEndian)
					reverseBytes((void*)&mByteRate, sizeof(mByteRate));

				// Bits per sample (2 bytes)
				pFStream.read((char*)&mBitsPerSample, 2);
				if (machineIsBigEndian)
					reverseBytes((void*)&mBitsPerSample, sizeof(mBitsPerSample));

				// Skip any extension bytes in the fmt chunk (chunkSize may be > 16)
				const int32_t extraBytes = chunkSize - 16;
				if (extraBytes > 0)
					pFStream.seekg(extraBytes, std::ios_base::cur);

				foundFmt = true;
			}
			else if (chunkID[0] == 'd' && chunkID[1] == 'a' &&
			         chunkID[2] == 't' && chunkID[3] == 'a')
			{
				// ── data chunk ──────────────────────────────────────────────────
				mDataSizeBytes = chunkSize;
				// Record the stream position at which audio samples begin.
				mAudioDataOffset = static_cast<int32_t>(pFStream.tellg());
				foundData = true;
				// Do not advance past the data — the caller reads samples from here.
			}
			else
			{
				// ── unknown chunk — skip it ──────────────────────────────────────
				// RIFF chunks are word-aligned: odd-sized chunks have a padding byte.
				int32_t skipBytes = chunkSize;
				if (skipBytes % 2 != 0)
					skipBytes++;
				pFStream.seekg(skipBytes, std::ios_base::cur);
			}
		}

		if (!foundFmt)
			result.addError("WAVFileInfo::read(): fmt chunk not found");
		else if (!foundData)
			result.addError("WAVFileInfo::read(): data chunk not found");

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
		// Length of format chunk (always 16 for standard PCM)
		data_4_bytes = 16;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		// Audio format (2 bytes): 1 = PCM
		int16_t data_2_bytes = 1;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		// Number of channels (2 bytes)
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
		// Block align (2 bytes)
		data_2_bytes = mByteRate;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));
		// Bits per sample (2 bytes)
		data_2_bytes = mBitsPerSample;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_2_bytes, sizeof(data_2_bytes));
		pFStream.write((const char*)&data_2_bytes, sizeof(data_2_bytes));

		// Data chunk header
		// "data" (ASCII characters)
		pFStream.write("data", 4);
		// Length of data to follow (4 bytes) — filled in on close()
		data_4_bytes = mDataSizeBytes;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));

		// Total header written: 44 bytes

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
		pOutFStream.seekp(4, std::ios_base::beg);   // file-size field is at byte 4
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

		// The data-chunk size field is at byte 40 in a standard 44-byte PCM header.
		pOutFStream.seekp(40, std::ios_base::beg);
		int32_t data_4_bytes = pDataSize;
		if (machineIsBigEndian)
			reverseBytes((void*)&data_4_bytes, sizeof(data_4_bytes));
		pOutFStream.write((const char*)&data_4_bytes, sizeof(data_4_bytes));
		return result;
	}

	string WAVFileInfo::WAVHeader() const
	{
		return string(mWAVHeader, sizeof(mWAVHeader));
	}

	string WAVFileInfo::RIFFType() const
	{
		return string(mRIFFType, sizeof(mRIFFType));
	}

	string WAVFileInfo::Subchunk2ID() const
	{
		return string(mSubchunk2ID, sizeof(mSubchunk2ID));
	}

	int32_t WAVFileInfo::Subchunk2Size() const
	{
		return mSubchunk2Size;
	}

	int32_t WAVFileInfo::DataSizeBytes() const
	{
		return mDataSizeBytes;
	}

	int32_t WAVFileInfo::AudioDataOffset() const
	{
		return mAudioDataOffset;
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
		return 44; // standard PCM WAV header: 44 bytes
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
