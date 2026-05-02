#ifndef __EO_UTILS_SNDFILE_HELPERS_H__
#define __EO_UTILS_SNDFILE_HELPERS_H__

#include <cstdint>
#include <vector>

#include <sndfile.h>

#include "AudioFileInfo.h"
#include "AudioFileResultType.h"

namespace EOUtils
{
	namespace sndfile_detail
	{
		/**
		 * @brief Fills AudioFileInfo from libsndfile SF_INFO and optional file size.
		 */
		void sfInfoToAudioFileInfo(const SF_INFO& pSf, std::uintmax_t pFileSizeBytes, AudioFileInfo& pOut);

		/**
		 * @brief Nominal bits per sample for decoded/interleaved int I/O via libsndfile.
		 */
		int16_t effectiveBitsPerSample(int pSfFormatFull);

		/**
		 * @brief Maximum PCM magnitude for clamping ints passed to sf_writef_*.
		 */
		int64_t maxSampleValueForFormat(int16_t pBitsPerSample);

		std::string formatErrorSuffix(SNDFILE* pSndOrNull);

		bool formatHasMajorMinor(int pFullFormat, int pMajorMask, int pMinorMask);

		std::uintmax_t safeFileSize(const std::string& pPath);

		std::vector<char> peekFilePrefix(const std::string& pPath, std::size_t pMaxLen);
	}
}

#endif
