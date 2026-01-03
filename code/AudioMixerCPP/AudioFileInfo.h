#ifndef __EO_UTILS_AUDIO_FILE_INFO_H__
#define __EO_UTILS_AUDIO_FILE_INFO_H__

#include <string>
#include <fstream>

#include "AudioFileResultType.h"

namespace EOUtils
{
	#define BITS_PER_BYTE 8

	class AudioFileInfo
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
			AudioFileInfo(int16_t pNumChannels = 0, int32_t pSampleRateHz = 0, int32_t pBytesPerSecond = 0, int16_t pByteRate = 0, int16_t pBitsPerSample = 0);

			/**
			 * @brief Copies the values from another AudioFileInfo object
			 *
			 * @param[in] pAudioFileInfo An AudioFileInfo object to copy the values from
			 */
			virtual void copyAudioFileInfo(const AudioFileInfo& pAudioFileInfo);

			/**
			 * @brief Reads the info from a WAV file
			 *
			 * @param[in] pFilename The name of a WAV file to read from
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success
			 */
			virtual AudioFileResultType read(const char *pFilename);

			/**
			 * @brief Reads the info from a file stream
			 *
			 * @param[in] pInFStream A file stream object to read the information from.  This must be already opened in read and binary mode.
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success
			 */
			// This would be pure virtual, but sometimes it has handy to instantiate objects of this class.
			virtual AudioFileResultType read(std::fstream& pInFStream);

			/**
			 * @brief Writes the info to a file stream
			 *
			 * @param[in] pOutFStream A file stream object to write the information to.  This must be already opened in write and binary mode.
			 *
			 * @return An AudioFileResultType object containing error messages on failure, or no errors on success
			 */
			 // This would be pure virtual, but sometimes it has handy to instantiate objects of this class.
			virtual AudioFileResultType write(std::fstream& pOutFStream);

			virtual int32_t FileSize() const;

			void FileSize(int32_t pFileSize);

			virtual int16_t NumChannels() const;

			virtual void NumChannels(int16_t pNumChannels);

			virtual int32_t SampleRateHz() const;

			virtual void SampleRateHz(int32_t pSampleRateHz);

			virtual int32_t BytesPerSecond() const;

			virtual void BytesPerSecond(int32_t pBytesPerSecond);

			virtual int16_t ByteRate() const;

			virtual void ByteRate(int16_t pByteRate);

			virtual int16_t BitsPerSample() const;

			virtual void BitsPerSample(int16_t pBitsPerSample);

			virtual size_t BytesPerSample() const;

		protected:
			int32_t mFileSize;       // File size
			int16_t mNumChannels;    // The # of channels (1 or 2)
			int32_t mSampleRateHz;   // The audio sample rate (Hz)
			int32_t mBytesPerSecond;
			int16_t mByteRate;
			int16_t mBitsPerSample;  // # of bits per sample
	};
}

#endif
