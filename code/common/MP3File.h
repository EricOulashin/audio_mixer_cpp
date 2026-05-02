#ifndef __EO_UTILS_MP3_FILE_H__
#define __EO_UTILS_MP3_FILE_H__

#include <cstdint>

#include <string>
#include <utility>

#include "SndFileBackedMetadataAudioFile.h"
#include "MP3FileInfo.h"

namespace EOUtils
{
	/** Matches SF_BITRATE_MODE_* (libsndfile LAME MPEG layer 3). */
	enum class Mp3BitrateMode : int
	{
		Constant = SF_BITRATE_MODE_CONSTANT,
		Average = SF_BITRATE_MODE_AVERAGE,
		Variable = SF_BITRATE_MODE_VARIABLE
	};

	class MP3File : public SndFileBackedMetadataAudioFile
	{
		public:
			explicit MP3File(const std::string& pFilename);

			explicit MP3File(const std::string& pFilename, const MP3FileInfo& pFileInfo);

			explicit MP3File(const std::string& pFilename, AudioFileModes pFileMode);

			/**
			 * @brief Open with write-time encoding options (constant or average bit rate).
			 * @param pNominalBitrateKbps Target kbps for CBR (Constant) or mean for ABR (Average). Not used for Variable.
			 */
			explicit MP3File(const std::string& pFilename, Mp3BitrateMode pBitrateMode, std::uint32_t pNominalBitrateKbps);

			/**
			 * @brief Variable bit rate: libsndfile compression in [0,1] (0 = best quality, 1 = lowest).
			 *        Maps to LAME VBR quality as in SFC_SET_COMPRESSION_LEVEL for SF_BITRATE_MODE_VARIABLE.
			 */
			explicit MP3File(const std::string& pFilename, double pVariableBitrateCompressionZeroBestOneWorst);

			MP3File(const MP3File& pOther);

			MP3File(MP3File&& pOther) noexcept;

			~MP3File() override;

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			const MP3FileInfo& getFileInfo() const;

			void setMp3WriteBitrateMode(Mp3BitrateMode pMode);

			void setMp3WriteNominalBitrateKbps(std::uint32_t pBitrateKbps);

			/** For Mp3BitrateMode::Variable — same semantics as constructor (0 best, 1 worst). */
			void setMp3WriteVariableBitrateCompression(double pZeroBestOneWorst);

			void clearMp3WriteEncodingOverrides();

			bool BitrateIsAdjustable() const override;

		protected:
			bool readFormatMatches(int pSfFullFormat) const override;

			int newFileSfFormatMask() const override;

			const char* formatLabel() const override;

			void configureSndfileEncoderForWrite(SNDFILE* pSnd) override;

		private:
			MP3FileInfo mMp3Info;
			bool mMp3CustomizeWriteEncoding = false;
			Mp3BitrateMode mMp3WriteMode = Mp3BitrateMode::Constant;
			std::uint32_t mMp3NominalBitrateKbps = 192;
			double mMp3VariableCompression = 0.35;
	};
}

#endif
