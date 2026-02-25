#ifndef __EO_UTILS_FLAC_FILE_INFO_H__
#define __EO_UTILS_FLAC_FILE_INFO_H__

#include <string>
#include <fstream>
#include <cstdint>

#include "AudioFileResultType.h"
#include "AudioFileInfo.h"

namespace EOUtils
{
	/**
	 * @brief Holds metadata for FLAC audio files.
	 */
	class FLACFileInfo : public AudioFileInfo
	{
		public:
			/**
			 * @brief Constructor
			 */
			FLACFileInfo(int16_t pNumChannels = 0, int32_t pSampleRateHz = 0, int32_t pBytesPerSecond = 0, int16_t pByteRate = 0, int16_t pBitsPerSample = 0);

			/**
			 * @brief Constructor for copying an AudioFileInfo object
			 */
			FLACFileInfo(const AudioFileInfo& pAudioFileInfo);

			/**
			 * @brief Constructor for copying a FLACFileInfo object
			 */
			FLACFileInfo(const FLACFileInfo& pAudioFileInfo);

			/**
			 * @brief Reads the FLAC stream info from a file
			 */
			AudioFileResultType read(std::fstream& pInFStream) override;

			/**
			 * @brief Not used for FLAC - metadata is written by the encoder
			 */
			AudioFileResultType write(std::fstream& pOutFStream) override;

			/**
			 * @brief Reads FLAC stream info from a file by filename
			 */
			AudioFileResultType read(const char* pFilename);

			/**
			 * @brief Check if the given file is a FLAC file (has "fLaC" signature)
			 */
			static bool isFLACFile(const char* pFilename);

			/**
			 * @brief Returns the current compression level
			 */
			uint32_t CompressionLevel() const;

			/**
			 * @brief Sets the compression level to use when writing FLAC files (0-8, where 0 is fastest and 8 is maximum compression)
			 */
			void CompressionLevel(uint32_t pCompressionLevel);

		private:
			uint32_t mCompressionLevel = 8; // Maximum compression
	};
}

#endif
