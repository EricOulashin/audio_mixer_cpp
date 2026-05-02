#ifndef __EO_UTILS_OGG_FILE_H__
#define __EO_UTILS_OGG_FILE_H__

#include <cstdint>

#include <string>
#include <utility>

#include "SndFileBackedMetadataAudioFile.h"
#include "OggFileInfo.h"

namespace EOUtils
{
	/** Vorbis write quality scale 1 best → 0 worst (mapped to libsndfile SFC_SET_COMPRESSION_LEVEL). */
	struct OggWriteQuality
	{
		explicit OggWriteQuality(double pQualityBestOneWorstZero);

		double QualityBestOneWorstZero;
	};

	/**
	 * Request an approximate nominal stream bitrate when writing Vorbis — libsndfile only exposes
	 * quality-based VBR; this converts kbps using a heuristic (typical stereo layout).
	 */
	struct OggWriteApproxBitrateKbps
	{
		explicit OggWriteApproxBitrateKbps(std::uint32_t pApproxNominalBitrateKbps);

		std::uint32_t ApproxNominalBitrateKbps;
	};

	inline OggWriteQuality::OggWriteQuality(double pQualityBestOneWorstZero)
		: QualityBestOneWorstZero(pQualityBestOneWorstZero)
	{
	}

	inline OggWriteApproxBitrateKbps::OggWriteApproxBitrateKbps(std::uint32_t pApproxNominalBitrateKbps)
		: ApproxNominalBitrateKbps(pApproxNominalBitrateKbps)
	{
	}

	class OggFile : public SndFileBackedMetadataAudioFile
	{
		public:
			explicit OggFile(const std::string& pFilename);

			explicit OggFile(const std::string& pFilename, const OggFileInfo& pFileInfo);

			explicit OggFile(const std::string& pFilename, AudioFileModes pFileMode);

			explicit OggFile(const std::string& pFilename, OggWriteQuality pQuality);

			explicit OggFile(const std::string& pFilename, OggWriteApproxBitrateKbps pApproxBitrate);

			OggFile(const OggFile& pOther);

			OggFile(OggFile&& pOther) noexcept;

			~OggFile() override;

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			const OggFileInfo& getFileInfo() const;

			void setOggWriteQualityBestOneWorstZero(double pQuality);

			void setOggWriteApproxNominalBitrateKbps(std::uint32_t pKbps);

			void clearOggWriteEncodingOverrides();

			bool BitrateIsAdjustable() const override;

		protected:
			bool readFormatMatches(int pSfFullFormat) const override;

			int newFileSfFormatMask() const override;

			const char* formatLabel() const override;

			void configureSndfileEncoderForWrite(SNDFILE* pSnd) override;

		private:
			enum class OggWriteEncodeStyle : std::uint8_t
			{
				LibsndfileDefault = 0,
				QualityVbr,
				ApproxBitrateHeuristic,
			};

			OggFileInfo mOggInfo;
			OggWriteEncodeStyle mOggWriteEncodeStyle = OggWriteEncodeStyle::LibsndfileDefault;
			double mOggWriteQualityBestOne = 1.0;
			std::uint32_t mOggWriteApproxKbps = 192;
	};
}

#endif
