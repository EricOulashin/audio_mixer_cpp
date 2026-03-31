#ifndef __EO_UTILS_FLACFILE_H__
#define __EO_UTILS_FLACFILE_H__

#include <cstdint>

#include "MetadataAudioFile.h"
#include "FLACFileInfo.h"
#include "AudioFileResultType.h"
#include "EOUtils.h"

#include <string>
#include <fstream>
#include <vector>

namespace EOUtils
{
	class FLACFile : public MetadataAudioFile
	{
		public:
			/**
			 * @brief Constructor for opening an existing FLAC file for reading,
			 * which just takes the filename
			 *
			 * @param[in] pFilename The name of the FLAC file
			 */
			FLACFile(const std::string& pFilename);

			/**
			 * @brief Constructor for opening an existing FLAC file for reading,
			 * which takes the filename and a compression level
			 *
			 * @param[in] pFilename The name of the FLAC file
			 * @param[in] pCompressionLevel The compression level to use when writing FLAC files (0-8, where 0 is fastest and 8 is maximum compression)
			 */
			FLACFile(const std::string& pFilenam, uint32_t pCompressionLevel);

			/**
			 * @brief Constructor for opening an existing FLAC file for reading
			 *
			 * @param[in] pFilename The name of the FLAC file
			 * @param[in] pFLACFileInfo An object containing information about the new FLAC file
			 */
			FLACFile(const std::string& pFilename, const FLACFileInfo& pFLACFileInfo);

			/**
			 * @brief Constructor for opening a FLAC file for reading or writing
			 * @param[in] pFilename The name of the FLAC file
			 * @param[in] pFileMode The mode to open the file in (read, write, or read/write)
			 */
			FLACFile(const std::string& pFilename, AudioFileModes pFileMode);

			/**
			 * @brief Copy constructor
			 */
			FLACFile(const FLACFile& pFLACFile);

			/**
			 * @brief Move constructor
			 */
			FLACFile(FLACFile&& pFLACFile) noexcept;

			~FLACFile();

			/**
			 * @brief Sets the audio file info for the FLAC file
			 */
			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			/**
			 * @brief Returns the compression level currently set for the FLAC file
			 */
			uint32_t CompressionLevel() const;

			/**
			 * @brief Sets the compression level to use when writing FLAC files
			 *
			 * @param[in] pCompressionLevel The compression level to use when writing FLAC files (0-8, where 0 is fastest and 8 is maximum compression)
			 */
			void CompressionLevel(uint32_t pCompressionLevel);

			/**
			 * @brief Opens the audio file in the given mode (read, write, or read/write)
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success.
			 * This can also be cast to a bool.
			 */
			AudioFileResultType open(AudioFileModes pOpenMode) override;

			/**
			 * @brief Closes the FLAC file
			 */
			void close() override;

			/**
			 * @brief Returns whether the FLAC file is open (and also has its encoder and/or decoder initialized)
			 */
			bool isOpen() const override;

			/**
			 * @brief Returns a FLACFileInfo object with information about the FLAC file
			 */
			const FLACFileInfo& getFileInfo() const;

			/**
			 * @brief Gets the next sample from the file, cast to a 64-bit integer
			 *
			 * @param[out] pAudioSample The next audio sample from the file, cast to a 64-bit integer
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success.
			 * This can also be cast to a bool.
			 */
			AudioFileResultType getNextSample_int64(int64_t& pAudioSample) override;

			/**
			 * @brief Writes an audio sample to the file.  The parameter is a 64-bit integer but will be cast to the
			 *  bitness of the audio samples in the file.
			 *
			 * @param[in] pAudioSample The audio sample to write to the file
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success.
			 * This can also be cast to a bool.
			 */
			AudioFileResultType writeSample_int64(int64_t pAudioSample) override;

			/**
			 * @brief Gets the highest audio sample value from the file, cast to a 64-bit integer
			 *
			 * @param[out] pHighestAudioSample The highest audio sample from the file, cast to a 64-bit integer
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success.
			 * This can also be cast to a bool.
			 */
			AudioFileResultType getHighestSampleValue_int64(int64_t& pHighestAudioSample) override;

			/**
			 * @brief Goes to the audio data position in the audio file
			 */
			AudioFileResultType goToAudioDataPos() override;

			/**
			 * @brief Returns the number of audio samples in the audio file
			 */
			size_t numSamples() const override;

			/**
			 * @brief Returns the maximum possible positive value of the audio file's sample size
			 */
			int64_t maxValueForSampleSize() const override;

			/**
			 * @brief Returns the maximum possible positive value of the audio file's sample size
			 */
			void seekOutputToSampleNum(size_t pSampleNum) override;

			/**
			 * @brief Returns an AudioFile object with information about the audio file
			 */
			AudioFileInfo getAudioFileInfo() const override;

			// MetadataAudioFile overrides for FLAC Vorbis Comment metadata
			AudioFileResultType setTitle(const std::string& pTitle) override;
			AudioFileResultType setArtist(const std::string& pArtist) override;
			AudioFileResultType setAlbum(const std::string& pAlbum) override;
			AudioFileResultType setGenre(const std::string& pGenre) override;
			AudioFileResultType setTrackNumber(uint32_t pTrackNumber, uint32_t pTotalTracks = 0) override;
			AudioFileResultType setYear(const std::string& pYear) override;
			AudioFileResultType setComment(const std::string& pComment) override;

			AudioFileResultType getTitle(std::string& pTitle) const override;
			AudioFileResultType getArtist(std::string& pArtist) const override;
			AudioFileResultType getAlbum(std::string& pAlbum) const override;
			AudioFileResultType getGenre(std::string& pGenre) const override;
			AudioFileResultType getTrackNumber(uint32_t& pTrackNumber, uint32_t& pTotalTracks) const override;
			AudioFileResultType getYear(std::string& pYear) const override;
			AudioFileResultType getComment(std::string& pComment) const override;

			/**
			 * @brief Used by FLAC decoder callbacks (same layout as FLAC client data)
			 */
			struct DecoderClientData {
				std::fstream* stream = nullptr;
				FLACFileInfo* fileInfo = nullptr;
				std::vector<std::vector<int32_t>>* readBuffer = nullptr;
				size_t* readChannel = nullptr;
				size_t* readSample = nullptr;
				AudioFileResultType* result = nullptr;
			};

		private:
			FLACFileInfo mFLACFileInfo;

			// FLAC decoder/encoder state (opaque pointers, avoid including FLAC headers in .h)
			void* mDecoder = nullptr;
			void* mEncoder = nullptr;

			// Sample buffer for reading (decoder outputs frames, we output samples one-by-one)
			// FLAC uses 32-bit signed samples; we store channels separately
			std::vector<std::vector<int32_t>> mReadBuffer;
			size_t mReadBufferChannel;
			size_t mReadBufferSample;

			// Write buffer for encoding (encoder expects interleaved blocks)
			std::vector<int32_t> mWriteBuffer;

			// For seeking in read mode
			uint64_t mDecodePosition;

			// Persistent client data for FLAC decoder callbacks (must outlive open())
			DecoderClientData mDecoderClientData;

			// Helpers for reading/writing FLAC Vorbis Comment metadata
			AudioFileResultType setVorbisComment(const std::string& pTagName, const std::string& pValue);
			AudioFileResultType getVorbisComment(const std::string& pTagName, std::string& pValue) const;
		};
}

#endif
