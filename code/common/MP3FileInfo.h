#ifndef __EO_UTILS_MP3_FILE_INFO_H__
#define __EO_UTILS_MP3_FILE_INFO_H__

#include "AudioFileInfo.h"
#include "AudioFileResultType.h"

namespace EOUtils
{
	class MP3FileInfo : public AudioFileInfo
	{
		public:
			MP3FileInfo(int16_t pNumChannels = 2, int32_t pSampleRateHz = 44100, int32_t pBytesPerSecond = 0,
			            int16_t pByteRate = 0, int16_t pBitsPerSample = 16);

			MP3FileInfo(const AudioFileInfo& pAudioFileInfo);

			AudioFileResultType read(std::fstream& pInFStream) override;

			AudioFileResultType read(const char* pFilename) override;

			AudioFileResultType write(std::fstream& pOutFStream) override;

			static bool matchesExtension(const char* pFilename);

			static bool sniffFileHeader(const char* pFilename);
	};
}

#endif
