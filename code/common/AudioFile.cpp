#include "AudioFile.h"

using std::string;
using std::map;
using std::ios_base;

namespace EOUtils
{
	AudioFile::AudioFile(const string& pFilename)
		: mFilename(pFilename),
		  mFileOpenMode(ios_base::binary),
		  mDataSizeBytes(0)
	{
	}

	AudioFile::AudioFile(const string& pFilename, AudioFileModes pFileMode)
		: mFilename(pFilename),
		  mFileOpenMode(ios_base::binary),
		  mDataSizeBytes(0)
	{
		open(pFileMode);
	}

	AudioFile::AudioFile(const AudioFile& pAudioFile)
		: mFilename(pAudioFile.mFilename),
		  //mReadMode(pAudioFile.mReadMode),
		  mFileOpenMode(pAudioFile.mFileOpenMode),
		  mDataSizeBytes(pAudioFile.mDataSizeBytes)
	{
	}

	AudioFile::~AudioFile()
	{
		// Ensure the audio file is closed
		close();
	}

	AudioFileResultType AudioFile::open(AudioFileModes pOpenMode)
	{
		AudioFileResultType result;
		setFileMode(pOpenMode);
		if (!hasReadMode() && !hasWriteMode())
		{
			result.addError("AudioFile::open() for " + mFilename + ": Open mode has neither read nor write mode");
			return result;
		}

		mFileStream.open(mFilename.c_str(), mFileOpenMode);
		if (!mFileStream.is_open())
			result.addError("AudioFile::open(): Unable to open " + mFilename);
		return result;
	}

	bool AudioFile::isOpen() const
	{
		return mFileStream.is_open();
	}

	bool AudioFile::hasReadMode() const
	{
		return ((mFileOpenMode & ios_base::in) == ios_base::in);
	}

	bool AudioFile::hasWriteMode() const
	{
		return ((mFileOpenMode & ios_base::out) == ios_base::out);
	}

	void AudioFile::close()
	{
		if (mFileStream.is_open())
			mFileStream.close();
	}

	const string& AudioFile::Filename() const
	{
		return mFilename;
	}

	void AudioFile::Filename(const string& pFilename)
	{
		mFilename = pFilename;
	}

	void AudioFile::setMetadata(const string& pName, const string& pValue)
	{
		mMetadata[pName] = pValue;
	}

	AudioFileResultType AudioFile::getMetadata(const string& pName, string& pValue) const
	{
		AudioFileResultType result;
		pValue = "";
		map<string, string>::const_iterator iterator = mMetadata.find(pName);
		if (iterator != mMetadata.end())
			pValue = iterator->second;
		else
			result.addError("Audio flie metadata with name " + pName + " was not found");
		return result;
	}

	string AudioFile::getMetadata(const string& pName) const
	{
		string value;
		map<string, string>::const_iterator iterator = mMetadata.find(pName);
		if (iterator != mMetadata.end())
			value = iterator->second;
		return value;
	}

	bool AudioFile::hasMetadata(const string& pName) const
	{
		return (mMetadata.find(pName) != mMetadata.end());
	}

	std::streampos AudioFile::fileSizeAccordingToStream()
	{
		const std::streampos currentPos = mFileStream.tellg();
		mFileStream.seekg(0, ios_base::beg);
		std::streampos fsize = mFileStream.tellg();
		mFileStream.seekg(0, ios_base::end);
		fsize = mFileStream.tellg() - fsize;
		mFileStream.seekg(currentPos, ios_base::beg);
		return fsize;
	}

	void AudioFile::setFileMode(AudioFileModes pFileMode)
	{
		mFileOpenMode = ios_base::binary;
		switch (pFileMode)
		{
			case AUDIO_FILE_READ:
				mFileOpenMode |= ios_base::in;
				break;
			case AUDIO_FILE_WRITE:
				mFileOpenMode |= ios_base::out;
				break;
			case AUDIO_FILE_READ_WRITE:
				mFileOpenMode |= (ios_base::in | ios_base::out);
				break;
		}
	}
}