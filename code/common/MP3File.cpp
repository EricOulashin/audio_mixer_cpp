#include "MP3File.h"
#include "SndFileHelpers.h"

#include <algorithm>
#include <stdexcept>

namespace EOUtils
{
	namespace
	{
		void bitrateExtremesForMp3(int pSampleRate, double& oMinKbps, double& oMaxKbps)
		{
			if (pSampleRate >= 32000)
			{
				oMinKbps = 32.0;
				oMaxKbps = 320.0;
			}
			else if (pSampleRate >= 16000)
			{
				oMinKbps = 8.0;
				oMaxKbps = 160.0;
			}
			else
			{
				oMinKbps = 8.0;
				oMaxKbps = 64.0;
			}
		}

		/** Inverse of libsndfile mpeg_l3_encoder_set_quality for non-VBR modes. */
		double mp3NominalKbpsToLibsfdCompression(std::uint32_t pBitrateKbps, int pSampleRate)
		{
			double mn = 32.0, mx = 320.0;
			bitrateExtremesForMp3(pSampleRate, mn, mx);
			const double clampedBr = std::clamp(static_cast<double>(pBitrateKbps), mn, mx);
			return (mx - clampedBr) / (mx - mn);
		}
	}

	MP3File::MP3File(const std::string& pFilename)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mMp3Info()
	{
		mMp3Info.copyAudioFileInfo(mFileLayout);
	}

	MP3File::MP3File(const std::string& pFilename, const MP3FileInfo& pFileInfo)
		: SndFileBackedMetadataAudioFile(pFilename, static_cast<const AudioFileInfo&>(pFileInfo)),
		  mMp3Info(pFileInfo)
	{
	}

	MP3File::MP3File(const std::string& pFilename, AudioFileModes pFileMode)
		: SndFileBackedMetadataAudioFile(pFilename, pFileMode),
		  mMp3Info()
	{
		mMp3Info.copyAudioFileInfo(mFileLayout);
	}

	MP3File::MP3File(const std::string& pFilename, Mp3BitrateMode pBitrateMode, std::uint32_t pNominalBitrateKbps)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mMp3Info(),
		  mMp3CustomizeWriteEncoding(true),
		  mMp3WriteMode(pBitrateMode),
		  mMp3NominalBitrateKbps(pNominalBitrateKbps)
	{
		if (pBitrateMode == Mp3BitrateMode::Variable)
			throw std::invalid_argument(
			    "MP3File: Mp3BitrateMode::Variable constructor requires floating-point compression; use "
			    "MP3File(path, compression) overload");
		mMp3Info.copyAudioFileInfo(mFileLayout);
	}

	MP3File::MP3File(const std::string& pFilename, double pVariableBitrateCompressionZeroBestOneWorst)
		: SndFileBackedMetadataAudioFile(pFilename),
		  mMp3Info(),
		  mMp3CustomizeWriteEncoding(true),
		  mMp3WriteMode(Mp3BitrateMode::Variable),
		  mMp3VariableCompression(pVariableBitrateCompressionZeroBestOneWorst)
	{
		mMp3Info.copyAudioFileInfo(mFileLayout);
	}

	MP3File::MP3File(const MP3File& pOther)
		: SndFileBackedMetadataAudioFile(pOther),
		  mMp3Info(pOther.mMp3Info),
		  mMp3CustomizeWriteEncoding(pOther.mMp3CustomizeWriteEncoding),
		  mMp3WriteMode(pOther.mMp3WriteMode),
		  mMp3NominalBitrateKbps(pOther.mMp3NominalBitrateKbps),
		  mMp3VariableCompression(pOther.mMp3VariableCompression)
	{
	}

	MP3File::MP3File(MP3File&& pOther) noexcept
		: SndFileBackedMetadataAudioFile(std::move(pOther)),
		  mMp3Info(std::move(pOther.mMp3Info)),
		  mMp3CustomizeWriteEncoding(pOther.mMp3CustomizeWriteEncoding),
		  mMp3WriteMode(pOther.mMp3WriteMode),
		  mMp3NominalBitrateKbps(pOther.mMp3NominalBitrateKbps),
		  mMp3VariableCompression(pOther.mMp3VariableCompression)
	{
	}

	MP3File::~MP3File() = default;

	void MP3File::setMp3WriteBitrateMode(Mp3BitrateMode pMode)
	{
		mMp3CustomizeWriteEncoding = true;
		mMp3WriteMode = pMode;
	}

	void MP3File::setMp3WriteNominalBitrateKbps(std::uint32_t pBitrateKbps)
	{
		mMp3CustomizeWriteEncoding = true;
		mMp3NominalBitrateKbps = pBitrateKbps;
	}

	void MP3File::setMp3WriteVariableBitrateCompression(double pZeroBestOneWorst)
	{
		mMp3CustomizeWriteEncoding = true;
		mMp3VariableCompression = pZeroBestOneWorst;
		mMp3WriteMode = Mp3BitrateMode::Variable;
	}

	void MP3File::clearMp3WriteEncodingOverrides()
	{
		mMp3CustomizeWriteEncoding = false;
		mMp3WriteMode = Mp3BitrateMode::Constant;
		mMp3NominalBitrateKbps = 192;
		mMp3VariableCompression = 0.35;
	}

	bool MP3File::BitrateIsAdjustable() const
	{
		return true;
	}

	void MP3File::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		SndFileBackedMetadataAudioFile::setAudioFileInfo(pAudioFileInfo);
		mMp3Info.copyAudioFileInfo(pAudioFileInfo);
	}

	AudioFileResultType MP3File::open(AudioFileModes pOpenMode)
	{
		AudioFileResultType r = SndFileBackedMetadataAudioFile::open(pOpenMode);
		if (r)
			mMp3Info.copyAudioFileInfo(mFileLayout);
		return r;
	}

	const MP3FileInfo& MP3File::getFileInfo() const
	{
		return mMp3Info;
	}

	bool MP3File::readFormatMatches(int pSfFullFormat) const
	{
		return sndfile_detail::formatHasMajorMinor(pSfFullFormat, SF_FORMAT_MPEG, SF_FORMAT_MPEG_LAYER_III);
	}

	int MP3File::newFileSfFormatMask() const
	{
		return SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III;
	}

	const char* MP3File::formatLabel() const
	{
		return "MP3File";
	}

	void MP3File::configureSndfileEncoderForWrite(SNDFILE* pSnd)
	{
		if (!mMp3CustomizeWriteEncoding || !pSnd)
			return;
		const int sr = std::max(1, static_cast<int>(mFileLayout.SampleRateHz()));
		double compression = 0.0;
		if (mMp3WriteMode == Mp3BitrateMode::Variable)
			compression = std::clamp(mMp3VariableCompression, 0.0, 1.0);
		else
			compression = mp3NominalKbpsToLibsfdCompression(mMp3NominalBitrateKbps, sr);

		if (sf_command(pSnd, SFC_SET_COMPRESSION_LEVEL, &compression,
		               static_cast<int>(sizeof(compression)))
		    != SF_TRUE)
			return;

		int bitrateModeCmd = static_cast<int>(mMp3WriteMode);
		sf_command(pSnd, SFC_SET_BITRATE_MODE, &bitrateModeCmd, static_cast<int>(sizeof(bitrateModeCmd)));
	}
}
