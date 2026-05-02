#include "MetadataAudioFile.h"

using std::string;

namespace EOUtils
{
	MetadataAudioFile::MetadataAudioFile(const string& pFilename)
		: AudioFile(pFilename)
	{
	}

	MetadataAudioFile::MetadataAudioFile(const string& pFilename, AudioFileModes pFileMode)
		: AudioFile(pFilename, pFileMode)
	{
	}

	MetadataAudioFile::MetadataAudioFile(const MetadataAudioFile& pOther)
		: AudioFile(pOther)
	{
	}

	MetadataAudioFile::~MetadataAudioFile()
	{
	}

	bool MetadataAudioFile::BitrateIsAdjustable() const
	{
		return false;
	}
}
