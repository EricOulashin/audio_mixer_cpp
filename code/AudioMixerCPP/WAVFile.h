#ifndef __EO_UTILS_WAVFILE_H__
#define __EO_UTILS_WAVFILE_H__

#include "AudioFile.h"
#include "WAVFileInfo.h"
#include "AudioFileResultType.h"
#include "EOUtils.h"
#include <string>
#include <fstream>
#include <sstream>


namespace EOUtils
{
	class WAVFile : public AudioFile
	{
		public:
			WAVFile(const std::string& pFilename);

			WAVFile(const std::string& pFilename, const WAVFileInfo& pWAVFileInfo);

			WAVFile(const std::string& pFilename, AudioFileModes pFileMode);

			WAVFile(const WAVFile& pWAVFile);

			~WAVFile();

			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			/**
			 * @brief Closes the WAV file
			 */
			void close() override;

			/**
			 * @brief Returns a WAVFileInfo object with information about the WAV file
			 */
			const WAVFileInfo& getFileInfo() const;

			/**
			 * @brief Gets the next sample from the file, cast to a 64-bit integer
			 *
			 * @param[out] pAudioSample The next audio sample from the file, cast to a 64-bit integer
			 *
			 * @return True on success, or false with error messages on failure
			 */
			AudioFileResultType getNextSample_int64(int64_t& pAudioSample) override;

			/**
			 * @brief Writes an audio sample to the file.  The parameter is a 64-bit integer but will be cast to the
			 *  bitness of the audio samples in the file.
			 *
			 * @param[in] pAudioSample The audio sample to write to the file
			 *
			 * @return True on success, or false with error messages on failure
			 */
			AudioFileResultType writeSample_int64(int64_t pAudioSample) override;

			/**
			 * @brief Gets the highest audio sample value from the file, cast to a 64-bit integer
			 *
			 * @param[out] pHighestAudioSample The highest audio sample from the file, cast to a 64-bit integer
			 *
			 * @return True on success, or false with error messages on failure
			 */
			AudioFileResultType getHighestSampleValue_int64(int64_t& pHighestAudioSample) override;

			/**
			 * @brief Gets the next audio sample from the file.  This is templated so that the proper
			 *  variable type can be used to store the audio sample value.
			 *
			 * @param[out] pAudioSample Templatized variable to store the next audio sample read from the file
			 *
			 * @return True on success, or false with error messages on failure
			 */
			template <class SampleType>
			AudioFileResultType getNextSample(SampleType& pAudioSample)
			{
				pAudioSample = 0;

				AudioFileResultType result;
				if (!hasReadMode())
				{
					result.addError("Can't get next audio file sample: Not in read mode");
					return result;
				}

				// If the audio sample size is more than the size of the data type being used, then return an error.
				if ((size_t)(mWAVFileInfo.BitsPerSample() / BITS_PER_BYTE) > sizeof(pAudioSample))
				{
					std::ostringstream oss;
					oss << "Audio sample size (" << mWAVFileInfo.BitsPerSample() << " bits) is more than data type size (" << (sizeof(SampleType) * BITS_PER_BYTE) << " bits)";
					result.addError(oss.str());
					return result;
				}

				// If the file isn't open, then return an error
				if (!mFileStream.is_open())
				{
					result.addError("WAVFile::getNextSample(): The file is not open");
					return result;
				}

				if (mFileStream.eof())
				{
					result.addError("At end of file");
					return result;
				}

				// Note: Right after opening the file, the header should have been read, so the file should
				// be ready for reading a sample.
				const size_t sampleSize = (mWAVFileInfo.BytesPerSample() < sizeof(SampleType) ? mWAVFileInfo.BytesPerSample() : sizeof(SampleType));
				mFileStream.read((char*)&pAudioSample, sampleSize);
				if ((sizeof(pAudioSample) > 1) && machineIsBigEndian)
					reverseBytes((void*)&pAudioSample, sizeof(pAudioSample));

				return result;
			}

			/**
			 * @brief Writes an audio sample to the file.  This is templated so that the proper variable type can be
			 *  used for the audio sample value.
			 *
			 * @param[in] pAudioSample Templatized variable containing the audio sample value to write to the file
			 *
			 * @return True on success, or false with error messages on failure
			 */
			template <class SampleType>
			AudioFileResultType writeSample(SampleType pAudioSample)
			{
				AudioFileResultType result;
				if (!hasWriteMode())
				{
					result.addError("Can't write audio sample: Not in write mode");
					return result;
				}

				if (mFileStream.is_open())
				{
					if ((sizeof(pAudioSample) > 1) && machineIsBigEndian)
						reverseBytes((void*)&pAudioSample, sizeof(pAudioSample));
					const size_t sampleSize = (mWAVFileInfo.BytesPerSample() < sizeof(pAudioSample) ? mWAVFileInfo.BytesPerSample() : sizeof(pAudioSample));
					mFileStream.write((const char*)&pAudioSample, sampleSize);
					mDataSizeBytes += sizeof(pAudioSample);
				}
				else
					result.addError("WAVFile::writeSample(): The file is not open");

				return result;
			}

			/**
			 * @brief Goes to the audio data position in the WAV file
			 */
			AudioFileResultType goToAudioDataPos() override;

			/**
			 * @brief Returns the number of audio samples in the WAV file
			 */
			size_t numSamples() const override;

			/**
			 * @brief Returns the maximum possible positive value of the audio file's sample size
			 */
			int64_t maxValueForSampleSize() const override;

			void seekOutputToSampleNum(size_t pSampleNum) override;

			/**
			 * @brief Gets the highest audio sample value from the file.  This is templated so that the proper
			 *  variable type can be used to store the audio sample value.
			 *
			 * @param[out] pAudioSample Templatized variable to store the highest audio sample read from the file
			 *
			 * @return True on success, or false with error messages on failure
			 */
			template <class SampleType>
			AudioFileResultType getHighestSampleValue(SampleType& pAudioSample)
			{
				pAudioSample = 0;

				AudioFileResultType result;
				if (!mFileStream.is_open())
				{
					result.addError("WAVFile::getHighestSamplevalue(): The file is not open");
					return result;
				}

				// Seek to the end of the WAV header and read through to find the highest sample value
				goToAudioDataPos();
				SampleType sampleValue = 0;
				SampleType highestSampleValue = 0;
				const size_t numAudioSamples = numSamples();
				for (size_t i = 0; (i < numAudioSamples) && result; ++i)
				{
					result = getNextSample(sampleValue);
					if (result)
					{
						if (sampleValue > highestSampleValue)
							highestSampleValue = sampleValue;
					}
				}

				if (result)
					pAudioSample = highestSampleValue;

				goToAudioDataPos();

				return result;
			}

			template <class SampleType>
			static AudioFileResultType getHighestSampleValue(const char* pFilename, SampleType& pAudioSample)
			{
				pAudioSample = 0;
				WAVFile inFile(pFilename);
				AudioFileResultType result = inFile.open(AUDIO_FILE_READ);
				if (result)
				{
					inFile.getHighestSampleValue(pAudioSample);
					inFile.close();
				}
				return result;
			}

			AudioFileInfo getAudioFileInfo() const override;


		private:
			WAVFileInfo mWAVFileInfo;
	};
}

#endif
