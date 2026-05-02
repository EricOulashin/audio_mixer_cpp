#include "AiffFile.h"
#include "SndFileHelpers.h"

namespace EOUtils
{
	namespace
	{
		int aiffPcmMaskForBits(int16_t pBits)
		{
			switch (pBits)
			{
				case 8:
					return SF_FORMAT_PCM_S8;
				case 24:
					return SF_FORMAT_PCM_24;
				case 32:
					return SF_FORMAT_PCM_32;
				case 16:
				default:
					return SF_FORMAT_PCM_16;
			}
		}
	}

	AiffFile::AiffFile(const std::string& pFilename)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mAiffInfo()
	{
		mAiffInfo.copyAudioFileInfo(mFileLayout);
	}

	AiffFile::AiffFile(const std::string& pFilename, const AiffFileInfo& pFileInfo)
		: SndFileBackedMetadataAudioFile(pFilename, static_cast<const AudioFileInfo&>(pFileInfo)),
		  mAiffInfo(pFileInfo)
	{
	}

	AiffFile::AiffFile(const std::string& pFilename, AudioFileModes pFileMode)
		: SndFileBackedMetadataAudioFile(pFilename, pFileMode),
		  mAiffInfo()
	{
		mAiffInfo.copyAudioFileInfo(mFileLayout);
	}

	AiffFile::AiffFile(const AiffFile& pOther)
		: SndFileBackedMetadataAudioFile(pOther),
		  mAiffInfo(pOther.mAiffInfo)
	{
	}

	AiffFile::AiffFile(AiffFile&& pOther) noexcept
		: SndFileBackedMetadataAudioFile(std::move(pOther)),
		  mAiffInfo(std::move(pOther.mAiffInfo))
	{
	}

	AiffFile::~AiffFile() = default;

	bool AiffFile::BitrateIsAdjustable() const
	{
		return false;
	}

	void AiffFile::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		SndFileBackedMetadataAudioFile::setAudioFileInfo(pAudioFileInfo);
		mAiffInfo.copyAudioFileInfo(pAudioFileInfo);
	}

	AudioFileResultType AiffFile::open(AudioFileModes pOpenMode)
	{
		AudioFileResultType r = SndFileBackedMetadataAudioFile::open(pOpenMode);
		if (r)
			mAiffInfo.copyAudioFileInfo(mFileLayout);
		return r;
	}

	const AiffFileInfo& AiffFile::getFileInfo() const
	{
		return mAiffInfo;
	}

	bool AiffFile::readFormatMatches(int pSfFullFormat) const
	{
		if ((pSfFullFormat & SF_FORMAT_TYPEMASK) != SF_FORMAT_AIFF)
			return false;
		const int sub = pSfFullFormat & SF_FORMAT_SUBMASK;
		return sub == SF_FORMAT_PCM_S8 || sub == SF_FORMAT_PCM_16 || sub == SF_FORMAT_PCM_24
		       || sub == SF_FORMAT_PCM_32;
	}

	int AiffFile::newFileSfFormatMask() const
	{
		return SF_FORMAT_AIFF | aiffPcmMaskForBits(mFileLayout.BitsPerSample());
	}

	const char* AiffFile::formatLabel() const
	{
		return "AiffFile";
	}
}
