#ifndef __EO_UTILS_WAV_FILE_INFO_H__
#define __EO_UTILS_WAV_FILE_INFO_H__

#include <string>
#include <fstream>

#include "AudioFileResultType.h"
#include "AudioFileInfo.h"

namespace EOUtils
{
	#define BITS_PER_BYTE 8

	class WAVFileInfo : public AudioFileInfo
	{
		public:
			/**
			 * @brief Constructor
			 *
			 * @param[in] pNumChannels The number of channels
			 * @param[in] pSampleRateHz The sample rate (in Hz)
			 * @param[in] pBytesPerSecond The number of bytes per second
			 * @param[in] pByteRate The byte rate (number of bytes per sample * number of channels)
			 * @param[in] pBitsPerSample The number of bits per sample
			 */
			WAVFileInfo(int16_t pNumChannels = 0, int32_t pSampleRateHz = 0, int32_t pBytesPerSecond = 0, int16_t pByteRate = 0, int16_t pBitsPerSample = 0);

			/**
			 * @brief Constructor for copying an AudioFileInfo object
			 *
			 * @param[in] pAudioFileInfo An AudioFileInfo object to copy the values from
			 */
			WAVFileInfo(const AudioFileInfo& pAudioFileInfo);

			/**
			 * @brief Reads the info from a file stream
			 *
			 * @param[in] pInFStream A file stream object to read the information from.  This must be already opened in read and binary mode.
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success
			 */
			AudioFileResultType read(std::fstream& pInFStream) override;

			/**
			 * @brief Writes the info to a file stream
			 *
			 * @param[in] pOutFStream A file stream object to write the information to.  This must be already opened in write and binary mode.
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success
			 */
			AudioFileResultType write(std::fstream& pOutFStream) override;

			AudioFileResultType updateFileSizeSizeInFile(std::fstream& pOutFStream, int32_t pFileSize);

			AudioFileResultType updateDataSizeSizeInFile(std::fstream& pOutFStream, int32_t pDataSize);

			std::string WAVHeader() const;

			std::string RIFFType() const;

			std::string Subchunk2ID() const;
			
			int32_t Subchunk2Size() const;

			int32_t DataSizeBytes() const;

			int16_t BitsPerSample() const;

			void BitsPerSample(int16_t pBitsPerSample);

			void SetWAVHeaderAndRIFFType();

			void SetSubchunk2IDAndSize();

			static size_t WAVFileHdrSize();

			static int16_t BitsPerSample(const char* pFilename);

			static bool isWAVFile(const char* pFilename);

		int32_t AudioDataOffset() const;

		private:
			char mWAVHeader[4];     // The WAV header (4 bytes, "RIFF")
			char mRIFFType[4];      // The RIFF type (4 bytes, "WAVE")
			char mSubchunk2ID[4];
			int32_t mSubchunk2Size;
			int32_t mDataSizeBytes;
			int32_t mAudioDataOffset;  // byte offset where audio sample data begins
	};
}

#endif
