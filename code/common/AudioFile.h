#ifndef __EO_UTILS_AUDIO_FILE_H__
#define __EO_UTILS_AUDIO_FILE_H__

#include <string>
#include <fstream>
#include <map>
#include <sstream>

#include "AudioFileResultType.h"
#include "AudioFileInfo.h"

namespace EOUtils
{
	enum AudioFileModes
	{
		AUDIO_FILE_READ,
		AUDIO_FILE_WRITE,
		AUDIO_FILE_READ_WRITE
	};

	class AudioFile
	{
		public:
			/**
			 * @brief Constructor
			 * @param[in] pFilename The name of the audio file
			 */
			explicit AudioFile(const std::string& pFilename);

			explicit AudioFile(const std::string& pFilename, AudioFileModes pFileMode);

			AudioFile(const AudioFile& pAudioFile);

			virtual ~AudioFile();

			virtual void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) = 0;

			virtual AudioFileResultType open(AudioFileModes pOpenMode);

			/**
			 * @brief Returns whether or not the audio file bitrate is adjustable
			 */
			virtual bool BitrateIsAdjustable() const = 0;

			/**
			 * @brief Returns whether or not the audio file is open
			 */
			virtual bool isOpen() const;

			/**
			 * @brief Returns whether or not read mode is enabled for the file
			 */
			virtual bool hasReadMode() const;

			/**
			 * @brief Returns whether or not write mode is enabled for the file
			 */
			virtual bool hasWriteMode() const;

			/**
			 * @brief Closes the audio file
			 */
			virtual void close();

			/**
			 * @brief Gets the next sample from the file, cast to a 64-bit integer
			 *
			 * @param[out] pAudioSample The next audio sample from the file, cast to a 64-bit integer
			 *
			 * @return True on success, or false with error messages on failure
			 */
			virtual AudioFileResultType getNextSample_int64(int64_t& pAudioSample) = 0;

			/**
			 * @brief Writes an audio sample to the file.  The parameter is a 64-bit integer but will be cast to the
			 *  bitness of the audio samples in the file.
			 *
			 * @param[in] pAudioSample The audio sample to write to the file
			 *
			 * @return True on success, or false with error messages on failure
			 */
			virtual AudioFileResultType writeSample_int64(int64_t pAudioSample) = 0;

			/**
			 * @brief Gets the highest audio sample value from the file, cast to a 64-bit integer
			 *
			 * @param[out] pHighestAudioSample The highest audio sample from the file, cast to a 64-bit integer
			 *
			 * @return True on success, or false with error messages on failure
			 */
			virtual AudioFileResultType getHighestSampleValue_int64(int64_t& pHighestAudioSample) = 0;

			/**
			 * @brief Goes to the audio data position in the audio file
			 */
			virtual AudioFileResultType goToAudioDataPos() = 0;

			/**
			 * @brief Returns the number of audio samples in the audio file
			 */
			virtual size_t numSamples() const = 0;

			/**
			 * @brief Returns the maximum possible positive value of the audio file's sample size
			 */
			virtual int64_t maxValueForSampleSize() const = 0;

			/**
			 * @brief Gets the filename of the audio file
			 */
			const std::string& Filename() const;

			/**
			 * @brief Setter for the filename of the audio file
			 *
			 * @param[in] pFilename The new filename
			 */
			void Filename(const std::string& pFilename);

			/**
			 * @brief Adds a piece of metadata for the audio file
			 *
			 * @param[in] pName The name of the metadata item (string)
			 * @param[in] pValue The value of the metadata item (string)
			 */
			void setMetadata(const std::string& pName, const std::string& pValue);

			/**
			 * @brief Adds a piece of metadata for the audio file.  Templatized for any value type.  The
			 *  value will be converted to a string to store in the metadata.
			 *
			 * @param[in] pName The name of the metadata item (string)
			 * @param[in] pValue The value of the metadata item (templatized for any type)
			 */
			template<typename T>
			void setMetadataFromVal(const std::string& pName, const T& pValue)
			{
				std::ostringstream oss;
				oss << pValue;
				mMetadata[pName] = oss.str();
			}

			/**
			 * @brief Gets a metadata value as a string
			 *
			 * @param[in] pName The name of the metadata item (string)
			 * @param[out] pValue This variable will store the value of the metadata item as a string
			 *
			 * @return True on success or false with an error if the item doesn't exist
			 */
			AudioFileResultType getMetadata(const std::string& pName, std::string& pValue) const;

			/**
			 * @brief Gets a metadata value.  Templatized to capture the value as any type.
			 *
			 * @param[in] pName The name of the metadata item (string)
			 * @param[out] pValue This variable will store the value of the metadata item.  Templatized to store the value as any type.
			 *
			 * @return True on success or false with an error if the item doesn't exist
			 */
			template<typename T>
			AudioFileResultType getMetadataAs(const std::string& pName, T& pValue) const
			{
				AudioFileResultType result;
				std::map<std::string, std::string>::const_iterator iterator = mMetadata.find(pName);
				if (iterator != mMetadata.end())
				{
					std::istringstream iss(iterator->second);
					iss >> pValue;
				}
				else
					result.addError("Audio flie metadata with name " + pName + " was not found");
				return result;
			}

			/**
			 * @brief Gets a metadata value
			 *
			 * @param[in] pName The name of the metadata item (string)
			 *
			 * @return The metadata item value, or a blank string if it doesn't exist
			 */
			std::string getMetadata(const std::string& pName) const;

			template<typename T>
			T getMetadataAs(const std::string& pName) const
			{
				T value{};
				std::map<std::string, std::string>::const_iterator iterator = mMetadata.find(pName);
				if (iterator != mMetadata.end())
				{
					std::istringstream iss(iterator->second);
					iss >> value;
				}
				return value;
			}

			/**
			 * @brief Gets a metadata value.  Templatized to return it as any type.
			 *
			 * @param[in] pName The name of the metadata item (string)
			 *
			 * @return The metadata item value, templatized as the given type
			 */
			template<typename T>
			T getMetadataAs(const std::string& pName, const T& pDefaultVal) const
			{
				T value = pDefaultVal;
				std::map<std::string, std::string>::const_iterator iterator = mMetadata.find(pName);
				if (iterator != mMetadata.end())
				{
					std::istringstream iss(iterator->second);
					iss >> value;
				}
				return value;
			}

			/**
			 * @brief Returns whether or not a metadata item exists.
			 *
			 * @param[in] pName The name of the metadata item to check
			 *
			 * @return True if the item exists, or false if not
			 */
			bool hasMetadata(const std::string& pName) const;

			/**
			 * @brief Returns an AudioFile object with information about the audio file
			 */
			virtual AudioFileInfo getAudioFileInfo() const = 0;

			virtual void seekOutputToSampleNum(size_t pSampleNum) = 0;

		protected:
			std::string mFilename;
			std::fstream mFileStream;
			std::ios_base::openmode mFileOpenMode;
			size_t mDataSizeBytes;
			std::map<std::string, std::string> mMetadata;

			virtual std::streampos fileSizeAccordingToStream();

			virtual void setFileMode(AudioFileModes pFileMode);
	};
}

#endif
