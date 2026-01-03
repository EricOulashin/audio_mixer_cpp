#include "AudioFileInfo.h"
#include "EOUtils.h"
using std::string;
using std::fstream;
using std::ifstream;


namespace EOUtils
{
	AudioFileInfo::AudioFileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond, int16_t pByteRate, int16_t pBitsPerSample)
		: mFileSize(0),
		  mNumChannels(pNumChannels),
		  mSampleRateHz(pSampleRateHz),
		  mBytesPerSecond(mBytesPerSecond),
		  mByteRate(pByteRate),
		  mBitsPerSample(pBitsPerSample)
	{
	}

	void AudioFileInfo::copyAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		mFileSize = pAudioFileInfo.mFileSize;
		mNumChannels = pAudioFileInfo.mNumChannels;
		mSampleRateHz = pAudioFileInfo.mSampleRateHz;
		mBytesPerSecond = pAudioFileInfo.mBytesPerSecond;
		mByteRate = pAudioFileInfo.mByteRate;
		mBitsPerSample = pAudioFileInfo.mBitsPerSample;
	}

	AudioFileResultType AudioFileInfo::read(const char *pFilename)
	{
		AudioFileResultType result;
		fstream inFile(pFilename, std::ios::binary | std::ios::in);
		if (inFile.is_open())
		{
			result = read(inFile);
			inFile.close();
		}
		else
			result.addError("Failed to open " + string(pFilename) + " for reading");
		return result;
	}

	// This would be pure virtual, but sometimes it has handy to instantiate objects of this class.
	AudioFileResultType AudioFileInfo::read(std::fstream& pInFStream)
	{
		AudioFileResultType result;
		result.addError("read(std::fstream&) called on instance of AudioFileInfo");
		return result;
	}

	// This would be pure virtual, but sometimes it has handy to instantiate objects of this class.
	AudioFileResultType AudioFileInfo::write(std::fstream& pOutFStream)
	{
		AudioFileResultType result;
		result.addError("write(std::fstream&) called on instance of AudioFileInfo");
		return result;
	}

	int32_t AudioFileInfo::FileSize() const
	{
		return mFileSize;
	}

	void AudioFileInfo::FileSize(int32_t pFileSize)
	{
		mFileSize = pFileSize;
	}

	int16_t AudioFileInfo::NumChannels() const
	{
		return mNumChannels;
	}

	void AudioFileInfo::NumChannels(int16_t pNumChannels)
	{
		mNumChannels = pNumChannels;
	}

	int32_t AudioFileInfo::SampleRateHz() const
	{
		return mSampleRateHz;
	}

	void AudioFileInfo::SampleRateHz(int32_t pSampleRateHz)
	{
		mSampleRateHz = pSampleRateHz;
	}

	int32_t AudioFileInfo::BytesPerSecond() const
	{
		return mBytesPerSecond;
	}

	void AudioFileInfo::BytesPerSecond(int32_t pBytesPerSecond)
	{
		mBytesPerSecond = pBytesPerSecond;
	}

	int16_t AudioFileInfo::ByteRate() const
	{
		return mByteRate;
	}

	void AudioFileInfo::ByteRate(int16_t pByteRate)
	{
		mByteRate = pByteRate;
	}

	int16_t AudioFileInfo::BitsPerSample() const
	{
		return mBitsPerSample;
	}

	void AudioFileInfo::BitsPerSample(int16_t pBitsPerSample)
	{
		mBitsPerSample = pBitsPerSample;
	}

	size_t AudioFileInfo::BytesPerSample() const
	{
		return((size_t)mBitsPerSample / BITS_PER_BYTE);
	}
}