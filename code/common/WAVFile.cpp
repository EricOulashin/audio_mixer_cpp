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

		const bool readMode = hasReadMode();
		const bool writeMode = hasWriteMode();
		if (readMode)
		{
			// Read-only mode or read & write mode - Read the file header if
			// the file is big enough.
			// If the file size is less than the standard WAV header size, then
			// return an error.
			if (initialFileSize < WAVFileInfo::WAVFileHdrSize())
			{
				mFileStream.close();
				std::ostringstream oss;
				oss << "The file size (" << initialFileSize << ") is less than a standard WAV header size (" << WAVFileInfo::WAVFileHdrSize() << ")";
				result.addError(oss.str());
				return result;
			}

			if (mFileStream.is_open())
			{
				result = mWAVFileInfo.read(mFileStream);
				if (result)
					mDataSizeBytes = initialFileSize - WAVFileInfo::WAVFileHdrSize();
			}
		}
		else if (!readMode && writeMode)
		{
			if (mWAVFileInfo.BitsPerSample() > 0)
				result = mWAVFileInfo.write(mFileStream);
			else
			{
				mFileStream.close();
				std::ostringstream oss;
				oss << "The specified sample size (" << mWAVFileInfo.BitsPerSample() << " bits) is <= 0";
				result.addError(oss.str());
			}
		}
		else if (!readMode && !writeMode)
			result.addError("WAVFile::open() for " + mFilename + ": Open mode has neither read nor write mode");

		return result;
	}

	void WAVFile::close()
	{
		if (mFileStream.is_open())
		{
			// If in write mode, set the file size in the header to file size according to stream - 8, then
			// update the file size in the WAV file before closing the stream
			if (hasWriteMode())
			{
				// Write the file size to the WAV file
				const int32_t fileSize = (int32_t)fileSizeAccordingToStream();
				// Note: Per the WAV file spec, we need to write file size - 8 bytes.
				// The header is 44 bytes, and 44 - 8 = 36
				mWAVFileInfo.updateFileSizeSizeInFile(mFileStream, fileSize - 8);

				// Write the data size to the WAV file
				mWAVFileInfo.updateDataSizeSizeInFile(mFileStream, fileSize - (int32_t)WAVFileInfo::WAVFileHdrSize());
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
			oss << "WAVFile::getNextSample_int64(): Unsupported number of bits per sample: " << mWAVFileInfo.BitsPerSample();
			result.addError(oss.str());
		}
		return result;
	}

	AudioFileResultType WAVFile::writeSample_int64(int64_t pAudioSample)
	{
		AudioFileResultType result;
		if (mWAVFileInfo.BitsPerSample() == 8)
			result = writeSample((uint8_t)pAudioSample);
		else if (mWAVFileInfo.BitsPerSample() == 16)
			result = writeSample((int16_t)pAudioSample);
		else if (mWAVFileInfo.BitsPerSample() == 32)
			result = writeSample((int32_t)pAudioSample);
		else
		{
			std::ostringstream oss;
			oss << "WAVFile::writeSample_int64(): Unsupported number of bits per sample: " << mWAVFileInfo.BitsPerSample();
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
			oss << "WAVFile::getHighestSampleValue_int64(): Unsupported number of bits per sample: " << mWAVFileInfo.BitsPerSample();
			result.addError(oss.str());
		}
		return result;
	}

	AudioFileResultType WAVFile::goToAudioDataPos()
	{
		AudioFileResultType result;
		if (mFileStream.is_open())
			mFileStream.seekg(WAVFileInfo::WAVFileHdrSize());
		else
			result.addError("WAVFile::goToAudioDataPos(): The file is not open");
		return result;
	}

	size_t WAVFile::numSamples() const
	{
		if (mWAVFileInfo.BytesPerSample() > 0)
			return(mDataSizeBytes / mWAVFileInfo.BytesPerSample());
		else
		{
			string msg = "WAVFile::numSamples() division by 0: Bytes per sample is 0 for " + mFilename;
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
			case 32:
				maxVal = EOUtils::maxValue<int32_t>();
				break;
		}
		return maxVal;
	}

	void WAVFile::seekOutputToSampleNum(size_t pSampleNum)
	{
		const int64_t numBytesOfMovement = pSampleNum * (int64_t)mWAVFileInfo.BytesPerSample();
		mFileStream.seekp(WAVFileInfo::WAVFileHdrSize() + numBytesOfMovement);
	}

	AudioFileInfo WAVFile::getAudioFileInfo() const
	{
		return mWAVFileInfo;
	}
}