#ifndef __EO_UTILS_SNDFILE_BACKED_METADATA_AUDIO_FILE_H__
#define __EO_UTILS_SNDFILE_BACKED_METADATA_AUDIO_FILE_H__

#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>

#include <sndfile.h>

#include "MetadataAudioFile.h"
#include "AudioFileResultType.h"

namespace EOUtils
{
	/**
	 * @brief Shared libsndfile-backed implementation used by MP3, Ogg Vorbis, and AIFF codecs.
	 */
	class SndFileBackedMetadataAudioFile : public MetadataAudioFile
	{
		public:
			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			void close() override;

			bool isOpen() const override;

			AudioFileResultType getNextSample_int64(std::int64_t& pAudioSample) override;

			AudioFileResultType writeSample_int64(std::int64_t pAudioSample) override;

			AudioFileResultType getHighestSampleValue_int64(std::int64_t& pHighestAudioSample) override;

			AudioFileResultType goToAudioDataPos() override;

			std::size_t numSamples() const override;

			std::int64_t maxValueForSampleSize() const override;

			void seekOutputToSampleNum(std::size_t pSampleNum) override;

			AudioFileInfo getAudioFileInfo() const override;

			AudioFileResultType setTitle(const std::string& pTitle) override;
			AudioFileResultType setArtist(const std::string& pArtist) override;
			AudioFileResultType setAlbum(const std::string& pAlbum) override;
			AudioFileResultType setGenre(const std::string& pGenre) override;
			AudioFileResultType setTrackNumber(std::uint32_t pTrackNumber, std::uint32_t pTotalTracks = 0) override;
			AudioFileResultType setYear(const std::string& pYear) override;
			AudioFileResultType setComment(const std::string& pComment) override;

			AudioFileResultType getTitle(std::string& pTitle) const override;
			AudioFileResultType getArtist(std::string& pArtist) const override;
			AudioFileResultType getAlbum(std::string& pAlbum) const override;
			AudioFileResultType getGenre(std::string& pGenre) const override;
			AudioFileResultType getTrackNumber(std::uint32_t& pTrackNumber, std::uint32_t& pTotalTracks) const override;
			AudioFileResultType getYear(std::string& pYear) const override;
			AudioFileResultType getComment(std::string& pComment) const override;

			~SndFileBackedMetadataAudioFile() override;

		protected:
			explicit SndFileBackedMetadataAudioFile(const std::string& pFilename);

			explicit SndFileBackedMetadataAudioFile(const std::string& pFilename, AudioFileModes pFileMode);

			SndFileBackedMetadataAudioFile(const SndFileBackedMetadataAudioFile& pOther);

			SndFileBackedMetadataAudioFile(SndFileBackedMetadataAudioFile&& pOther) noexcept;

			explicit SndFileBackedMetadataAudioFile(const std::string& pFilename, const AudioFileInfo& pLayoutHint);

			virtual bool readFormatMatches(int pSfFullFormat) const = 0;

			virtual int newFileSfFormatMask() const = 0;

			virtual const char* formatLabel() const = 0;

			/** After a successful SFM_WRITE open, codecs may tweak the encoder via sf_command. Default: no-op. */
			virtual void configureSndfileEncoderForWrite(SNDFILE* /*pSnd*/)
			{}

			AudioFileInfo mFileLayout;

		private:
			SNDFILE* mSnd = nullptr;
			SF_INFO mSf{};
			std::vector<int> mFrameBuf;
			std::size_t mChannelIndexInPump = 0;
			std::vector<int> mWriteAccum;

			void finalizePartialWriteFrames();

			AudioFileResultType getSfStringField(int pStrKind, std::string& pOut) const;

			AudioFileResultType setSfStringField(int pStrKind, const std::string& pVal);

			void syncLayoutBytesFromSf();

			void bumpDataSizeEstimate();

			void resetReadPump();
	};
}

#endif
