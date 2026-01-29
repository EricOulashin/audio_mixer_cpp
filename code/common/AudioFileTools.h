#ifndef __AUDIO_FILE_TOOLS_H__
#define __AUDIO_FILE_TOOLS_H__

#include <memory>
#include <string>
#include <vector>

#include "AudioFile.h"
#include "AudioFileInfo.h"
#include "AudioFileResultType.h"

namespace EOUtils
{
	/**
	 * @brief Creates a shared pointer to an audioFile object for an existing audio file - Creates the appropriate subclass object,
	 *   depending on the file format.
	 *
	 * @param[in] pFilename The name of the audio file
	 *
	 * @return A shared pointer to an AudioFile object, which will be an instance of the appropriate subclass for the file, or a null pointer if it is not a recognized format.
	 */
	std::shared_ptr<AudioFile> createAudioFileObj(const char* pFilename);

	/**
	 * @brief Gets information about an audio file
	 *
	 * @param[in] pFilename The name of the audio file
	 * @param[out] pAudioFileInfo An AudioFileInfo object that will be populated with information about the audio file
	 *
	 * @return True on success, or false with error messages on failure
	 */
	AudioFileResultType getAudioFileInfo(const char* pFilename, AudioFileInfo& pAudioFileInfo);

	/**
	 * @brief Mixes (merges) multiple audio files into a single file
	 *
	 * @param[in] pFilenames A collection of filenames of audio files to mix
	 * @param[in,out] pOutFile An AudioFile object representing the file to output to
	 *
	 * @return True on success, or false with error messages on failure
	 */
	AudioFileResultType mixAudioFiles(const std::vector<std::string>& pFilenames, AudioFile& pOutFile);

	/**
	 * @brief Gets the highest audio sample from a set of audio files, cast to a 64-bit integer
	 *
	 * @param[in] pFilenames A collection of audio file names
	 * @param[out] pHighestAudioSample This will contain the highest audio sample found in the audio files
	 *
	 * @return True on success, or false with error messages on failure
	 */
	AudioFileResultType getHighestSampleValue_64bit(const std::vector<std::string>& pFilenames, int64_t& pHighestAudioSample);
}

#endif