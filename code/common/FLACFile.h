#ifndef __EO_UTILS_FLACFILE_H__
#define __EO_UTILS_FLACFILE_H__

#include <cstdint>

#include "AudioFile.h"
#include "FLACFileInfo.h"
#include "AudioFileResultType.h"
#include "EOUtils.h"

#include <string>
#include <fstream>
#include <vector>

namespace EOUtils
{
	class FLACFile : public AudioFile
	{
		public:
			FLACFile(const std::string& pFilename);

			FLACFile(const std::string& pFilename, const FLACFileInfo& pFLACFileInfo);

			FLACFile(const std::string& pFilename, AudioFileModes pFileMode);

			FLACFile(const FLACFile& pFLACFile);

			~FLACFile();

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

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			void close() override;

			bool isOpen() const override;

			/**
			 * @brief Returns a FLACFileInfo object with information about the FLAC file
			 */
			const FLACFileInfo& getFileInfo() const;

			AudioFileResultType getNextSample_int64(int64_t& pAudioSample) override;

			AudioFileResultType writeSample_int64(int64_t pAudioSample) override;

			AudioFileResultType getHighestSampleValue_int64(int64_t& pHighestAudioSample) override;

			AudioFileResultType goToAudioDataPos() override;

			size_t numSamples() const override;

			int64_t maxValueForSampleSize() const override;

			void seekOutputToSampleNum(size_t pSampleNum) override;

			AudioFileInfo getAudioFileInfo() const override;

			virtual FLACFileInfo getFLACFileInfo() const;

			// Used by FLAC decoder callbacks (same layout as FLAC client data)
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
			void* mDecoder;
			void* mEncoder;

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
		};
}

#endif
