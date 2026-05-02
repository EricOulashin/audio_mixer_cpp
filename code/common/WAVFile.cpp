#include "WAVFile.h"
#include "utilFunctions.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

using std::logic_error;
using std::string;
using std::ios_base;

namespace EOUtils
{
	WAVFile::WAVFile(const string& pFilename)
		: AudioFile(pFilename)
	{
	}

	WAVFile::WAVFile(const string& pFilename, const WAVFileInfo& pWAVFileInfo)
		: AudioFile(pFilename),
		  mWAVFileInfo(pWAVFileInfo)
	{
	}

	WAVFile::WAVFile(const string& pFilename, AudioFileModes pFileMode)
		: AudioFile(pFilename, pFileMode)
	{
	}

	WAVFile::WAVFile(const WAVFile& pWAVFile)
		: AudioFile(pWAVFile),
		  mWAVFileInfo(pWAVFile.mWAVFileInfo)
	{
	}

	WAVFile::~WAVFile()
	{
		// Note: The parent class (AudioFile) destructor will call close()
	}

	bool WAVFile::BitrateIsAdjustable() const
	{
		return false;
	}

	void WAVFile::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		mWAVFileInfo.copyAudioFileInfo(pAudioFileInfo);
		mWAVFileInfo.SetWAVHeaderAndRIFFType();
		mWAVFileInfo.SetSubchunk2IDAndSize();
	}

	AudioFileResultType WAVFile::open(AudioFileModes pOpenMode)
	{
		if (mFileStream.is_open())
			mFileStream.close();
		mDataSizeBytes = 0;

		const size_t initialFileSize = getFileSize(mFilename.c_str());

		AudioFileResultType result = AudioFile::open(pOpenMode);
		if (!result)
		{
			mFileStream.close();
			return result;
		}

		const bool readMode  = hasReadMode();
		const bool writeMode = hasWriteMode();
		if (readMode)
		{
			// Read-only or read+write mode — read the file header.
			// Reject files that are obviously too small to contain a WAV header.
			if (initialFileSize < WAVFileInfo::WAVFileHdrSize())
			{
				mFileStream.close();
				std::ostringstream oss;
				oss << "The file size (" << initialFileSize
				    << ") is less than a standard WAV header size ("
				    << WAVFileInfo::WAVFileHdrSize() << ")";
				result.addError(oss.str());
				return result;
			}

			if (mFileStream.is_open())
			{
				result = mWAVFileInfo.read(mFileStream);
				if (result)
				{
					// Use the data size from the parsed header rather than
					// estimating it from file size, so extra trailing metadata
					// (or a truncated file) doesn't confuse numSamples().
					mDataSizeBytes = static_cast<size_t>(mWAVFileInfo.DataSizeBytes());
				}
			}
		}
		else if (!readMode && writeMode)
		{
			if (mWAVFileInfo.BitsPerSample() > 0)
			{
				result = mWAVFileInfo.write(mFileStream);
			}
			else
			{
				mFileStream.close();
				std::ostringstream oss;
				oss << "The specified sample size (" << mWAVFileInfo.BitsPerSample()
				    << " bits) is <= 0";
				result.addError(oss.str());
			}
		}
		else if (!readMode && !writeMode)
		{
			result.addError("WAVFile::open() for " + mFilename
			                + ": Open mode has neither read nor write mode");
		}

		return result;
	}

	void WAVFile::close()
	{
		if (mFileStream.is_open())
		{
			// If in write mode, update the file-size and data-size fields in the
			// WAV header before closing the stream.
			if (hasWriteMode())
			{
				const int32_t fileSize = (int32_t)fileSizeAccordingToStream();
				// Per the WAV spec, the RIFF file-size field holds (total bytes - 8).
				mWAVFileInfo.updateFileSizeSizeInFile(mFileStream, fileSize - 8);
				mWAVFileInfo.updateDataSizeSizeInFile(mFileStream,
				    fileSize - (int32_t)WAVFileInfo::WAVFileHdrSize());
			}
			mFileStream.close();
		}
	}

	const WAVFileInfo& WAVFile::getFileInfo() const
	{
		return mWAVFileInfo;
	}

	AudioFileResultType WAVFile::getNextSample_int64(int64_t& pAudioSample)
	{
		AudioFileResultType result;
		pAudioSample = 0;
		if (mWAVFileInfo.BitsPerSample() == 8)
		{
			uint8_t audioSample = 0;
			result = getNextSample(audioSample);
			if (result)
				pAudioSample = (int64_t)audioSample;
		}
		else if (mWAVFileInfo.BitsPerSample() == 16)
		{
			int16_t audioSample = 0;
			result = getNextSample(audioSample);
			if (result)
				pAudioSample = (int64_t)audioSample;
		}
		else if (mWAVFileInfo.BitsPerSample() == 24)
		{
			if (!mFileStream.is_open())
			{
				result.addError("WAVFile::getNextSample_int64(): The file is not open");
				return result;
			}
			if (mFileStream.eof())
			{
				result.addError("At end of file");
				return result;
			}
			// 24-bit PCM is stored as 3 little-endian bytes; sign-extend to 64 bits.
			uint8_t bytes[3] = {0, 0, 0};
			mFileStream.read(reinterpret_cast<char*>(bytes), 3);
			int32_t val = static_cast<int32_t>(bytes[0])
			            | (static_cast<int32_t>(bytes[1]) << 8)
			            | (static_cast<int32_t>(bytes[2]) << 16);
			if (val & 0x800000)
				val |= static_cast<int32_t>(0xFF000000);  // sign-extend
			pAudioSample = static_cast<int64_t>(val);
		}
		else if (mWAVFileInfo.BitsPerSample() == 32)
		{
			int32_t audioSample = 0;
			result = getNextSample(audioSample);
			if (result)
				pAudioSample = (int64_t)audioSample;
		}
		else
		{
			std::ostringstream oss;
			oss << "WAVFile::getNextSample_int64(): Unsupported number of bits per sample: "
			    << mWAVFileInfo.BitsPerSample();
			result.addError(oss.str());
		}
		return result;
	}

	AudioFileResultType WAVFile::writeSample_int64(int64_t pAudioSample)
	{
		AudioFileResultType result;
		if (mWAVFileInfo.BitsPerSample() == 8)
		{
			result = writeSample((uint8_t)pAudioSample);
		}
		else if (mWAVFileInfo.BitsPerSample() == 16)
		{
			result = writeSample((int16_t)pAudioSample);
		}
		else if (mWAVFileInfo.BitsPerSample() == 24)
		{
			if (!mFileStream.is_open())
			{
				result.addError("WAVFile::writeSample_int64(): The file is not open");
				return result;
			}
			// Clamp to signed 24-bit range [-8388608, 8388607]
			int32_t val = static_cast<int32_t>(
			    std::max(static_cast<int64_t>(-8388608),
			             std::min(static_cast<int64_t>(8388607), pAudioSample)));
			uint8_t bytes[3];
			bytes[0] = static_cast<uint8_t>(val & 0xFF);
			bytes[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
			bytes[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
			mFileStream.write(reinterpret_cast<const char*>(bytes), 3);
			mDataSizeBytes += 3;
		}
		else if (mWAVFileInfo.BitsPerSample() == 32)
		{
			result = writeSample((int32_t)pAudioSample);
		}
		else
		{
			std::ostringstream oss;
			oss << "WAVFile::writeSample_int64(): Unsupported number of bits per sample: "
			    << mWAVFileInfo.BitsPerSample();
			result.addError(oss.str());
		}
		return result;
	}

	AudioFileResultType WAVFile::getHighestSampleValue_int64(int64_t& pHighestAudioSample)
	{
		AudioFileResultType result;
		pHighestAudioSample = 0;
		if (mWAVFileInfo.BitsPerSample() == 8)
		{
			uint8_t highestAudioSample = 0;
			result = getHighestSampleValue(highestAudioSample);
			if (result)
				pHighestAudioSample = (int64_t)highestAudioSample;
		}
		else if (mWAVFileInfo.BitsPerSample() == 16)
		{
			int16_t highestAudioSample = 0;
			result = getHighestSampleValue(highestAudioSample);
			if (result)
				pHighestAudioSample = (int64_t)highestAudioSample;
		}
		else if (mWAVFileInfo.BitsPerSample() == 24)
		{
			goToAudioDataPos();
			const size_t numSmp = numSamples();
			int64_t highest = 0;
			for (size_t i = 0; i < numSmp && result; ++i)
			{
				int64_t sample = 0;
				result = getNextSample_int64(sample);
				if (result && sample > highest)
					highest = sample;
			}
			if (result)
				pHighestAudioSample = highest;
			goToAudioDataPos();
		}
		else if (mWAVFileInfo.BitsPerSample() == 32)
		{
			int32_t highestAudioSample = 0;
			result = getHighestSampleValue(highestAudioSample);
			if (result)
				pHighestAudioSample = (int64_t)highestAudioSample;
		}
		else
		{
			std::ostringstream oss;
			oss << "WAVFile::getHighestSampleValue_int64(): Unsupported number of bits per sample: "
			    << mWAVFileInfo.BitsPerSample();
			result.addError(oss.str());
		}
		return result;
	}

	AudioFileResultType WAVFile::goToAudioDataPos()
	{
		AudioFileResultType result;
		if (mFileStream.is_open())
		{
			// Use the actual data offset recorded when the header was parsed
			// (may be > 44 for WAV files with extra chunks before "data").
			mFileStream.seekg(static_cast<std::streamoff>(mWAVFileInfo.AudioDataOffset()));
		}
		else
		{
			result.addError("WAVFile::goToAudioDataPos(): The file is not open");
		}
		return result;
	}

	size_t WAVFile::numSamples() const
	{
		if (mWAVFileInfo.BytesPerSample() > 0)
			return (mDataSizeBytes / mWAVFileInfo.BytesPerSample());
		else
		{
			string msg = "WAVFile::numSamples() division by 0: Bytes per sample is 0 for "
			           + mFilename;
			throw std::logic_error(msg.c_str());
		}
	}

	int64_t WAVFile::maxValueForSampleSize() const
	{
		int64_t maxVal = 0;
		switch (mWAVFileInfo.BitsPerSample())
		{
			case 8:
				maxVal = EOUtils::maxValue<uint8_t>();
				break;
			case 16:
				maxVal = EOUtils::maxValue<int16_t>();
				break;
			case 24:
				maxVal = 8388607;  // 2^23 - 1
				break;
			case 32:
				maxVal = EOUtils::maxValue<int32_t>();
				break;
		}
		return maxVal;
	}

	void WAVFile::seekOutputToSampleNum(size_t pSampleNum)
	{
		const int64_t numBytesOfMovement = pSampleNum * (int64_t)mWAVFileInfo.BytesPerSample();
		mFileStream.seekp(mWAVFileInfo.AudioDataOffset() + numBytesOfMovement);
	}

	AudioFileInfo WAVFile::getAudioFileInfo() const
	{
		return mWAVFileInfo;
	}
}
