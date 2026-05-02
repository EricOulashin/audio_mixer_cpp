#include "OggFile.h"
#include "SndFileHelpers.h"

#include <algorithm>

namespace EOUtils
{
	namespace
	{
		/**
		 * Heuristic nominal kbps → libsndfile compression (lower compression = higher Vorbis encode quality).
		 * Tune is qualitative; mono gets a higher inferred target than stereo at the same nominal kbps.
		 */
		double oggApproxKbpsToLibsfdCompression(std::uint32_t pTotalStreamKbps,
		                                                 int16_t pNumChannels, int32_t pSampleRate)
		{
			(void)pSampleRate;
			const double ch =
			    std::max(1, static_cast<int>(pNumChannels));
			double brEff = static_cast<double>(pTotalStreamKbps);
			brEff *= 2.0 / ch;

			brEff = std::clamp(brEff, 56.0, 256.0);
			const double normalized = (brEff - 96.0) / (216.0 - 96.0);
			const double vorbisQuality = std::clamp(normalized, 0.0, 1.0);
			return 1.0 - vorbisQuality;
		}
	}

	OggFile::OggFile(const std::string& pFilename)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mOggInfo()
	{
		mOggInfo.copyAudioFileInfo(mFileLayout);
	}

	OggFile::OggFile(const std::string& pFilename, const OggFileInfo& pFileInfo)
		: SndFileBackedMetadataAudioFile(pFilename, static_cast<const AudioFileInfo&>(pFileInfo)),
		  mOggInfo(pFileInfo)
	{
	}

	OggFile::OggFile(const std::string& pFilename, AudioFileModes pFileMode)
		: SndFileBackedMetadataAudioFile(pFilename, pFileMode),
		  mOggInfo()
	{
		mOggInfo.copyAudioFileInfo(mFileLayout);
	}

	OggFile::OggFile(const std::string& pFilename, OggWriteQuality pQuality)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mOggInfo(),
		  mOggWriteEncodeStyle(OggWriteEncodeStyle::QualityVbr),
		  mOggWriteQualityBestOne(pQuality.QualityBestOneWorstZero)
	{
		mOggInfo.copyAudioFileInfo(mFileLayout);
	}

	OggFile::OggFile(const std::string& pFilename, OggWriteApproxBitrateKbps pApproxBitrate)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mOggInfo(),
		  mOggWriteEncodeStyle(OggWriteEncodeStyle::ApproxBitrateHeuristic),
		  mOggWriteApproxKbps(pApproxBitrate.ApproxNominalBitrateKbps)
	{
		mOggInfo.copyAudioFileInfo(mFileLayout);
	}

	OggFile::OggFile(const OggFile& pOther)
		: SndFileBackedMetadataAudioFile(pOther),
		  mOggInfo(pOther.mOggInfo),
		  mOggWriteEncodeStyle(pOther.mOggWriteEncodeStyle),
		  mOggWriteQualityBestOne(pOther.mOggWriteQualityBestOne),
		  mOggWriteApproxKbps(pOther.mOggWriteApproxKbps)
	{
	}

	OggFile::OggFile(OggFile&& pOther) noexcept
		: SndFileBackedMetadataAudioFile(std::move(pOther)),
		  mOggInfo(std::move(pOther.mOggInfo)),
		  mOggWriteEncodeStyle(pOther.mOggWriteEncodeStyle),
		  mOggWriteQualityBestOne(pOther.mOggWriteQualityBestOne),
		  mOggWriteApproxKbps(pOther.mOggWriteApproxKbps)
	{
	}

	OggFile::~OggFile() = default;

	void OggFile::setOggWriteQualityBestOneWorstZero(double pQuality)
	{
		mOggWriteEncodeStyle = OggWriteEncodeStyle::QualityVbr;
		mOggWriteQualityBestOne = pQuality;
	}

	void OggFile::setOggWriteApproxNominalBitrateKbps(std::uint32_t pKbps)
	{
		mOggWriteEncodeStyle = OggWriteEncodeStyle::ApproxBitrateHeuristic;
		mOggWriteApproxKbps = pKbps;
	}

	void OggFile::clearOggWriteEncodingOverrides()
	{
		mOggWriteEncodeStyle = OggWriteEncodeStyle::LibsndfileDefault;
		mOggWriteQualityBestOne = 1.0;
		mOggWriteApproxKbps = 192;
	}

	bool OggFile::BitrateIsAdjustable() const
	{
		return true;
	}

	void OggFile::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		SndFileBackedMetadataAudioFile::setAudioFileInfo(pAudioFileInfo);
		mOggInfo.copyAudioFileInfo(pAudioFileInfo);
	}

	AudioFileResultType OggFile::open(AudioFileModes pOpenMode)
	{
		AudioFileResultType r = SndFileBackedMetadataAudioFile::open(pOpenMode);
		if (r)
			mOggInfo.copyAudioFileInfo(mFileLayout);
		return r;
	}

	const OggFileInfo& OggFile::getFileInfo() const
	{
		return mOggInfo;
	}

	bool OggFile::readFormatMatches(int pSfFullFormat) const
	{
		return sndfile_detail::formatHasMajorMinor(pSfFullFormat, SF_FORMAT_OGG, SF_FORMAT_VORBIS);
	}

	int OggFile::newFileSfFormatMask() const
	{
		return SF_FORMAT_OGG | SF_FORMAT_VORBIS;
	}

	const char* OggFile::formatLabel() const
	{
		return "OggFile";
	}

	void OggFile::configureSndfileEncoderForWrite(SNDFILE* pSnd)
	{
		if (!pSnd || mOggWriteEncodeStyle == OggWriteEncodeStyle::LibsndfileDefault)
			return;

		double compression = 0.0;
		if (mOggWriteEncodeStyle == OggWriteEncodeStyle::ApproxBitrateHeuristic)
		{
			compression = oggApproxKbpsToLibsfdCompression(mOggWriteApproxKbps,
			                                                   mFileLayout.NumChannels(),
			                                                   mFileLayout.SampleRateHz());
		}
		else if (mOggWriteEncodeStyle == OggWriteEncodeStyle::QualityVbr)
			compression = 1.0 - std::clamp(mOggWriteQualityBestOne, 0.0, 1.0);
		else
			return;

		compression = std::clamp(compression, 0.0, 1.0);
		sf_command(pSnd, SFC_SET_COMPRESSION_LEVEL, &compression,
		           static_cast<int>(sizeof(compression)));
	}
}
