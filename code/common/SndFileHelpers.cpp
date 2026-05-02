#include "SndFileHelpers.h"

#include <fstream>
#include <limits>
#include <sndfile.h>
#include <cstring>
#include <algorithm>
#include <string>

namespace EOUtils
{
	namespace sndfile_detail
	{
		namespace
		{
			int extractSubtype(int fullFormat)
			{
				return fullFormat & SF_FORMAT_SUBMASK;
			}

			std::streamoff fileSizeViaStream(const std::string& pPath)
			{
				std::ifstream in(pPath, std::ios::binary | std::ios::ate);
				if (!in.good())
					return -1;
				return static_cast<std::streamoff>(in.tellg());
			}
		}

		void sfInfoToAudioFileInfo(const SF_INFO& pSf, std::uintmax_t pFileSizeBytes, AudioFileInfo& pOut)
		{
			const int16_t bits = effectiveBitsPerSample(static_cast<int>(pSf.format));
			const int16_t nc = static_cast<int16_t>(pSf.channels);
			const int32_t sr = static_cast<int32_t>(pSf.samplerate);
			const int16_t bytesPerChan = static_cast<int16_t>(bits / 8);

			const int32_t bytesPerSecond = static_cast<int32_t>(static_cast<int32_t>(nc) * sr * bytesPerChan);
			pOut.FileSize(pFileSizeBytes <= static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max())
			                  ? static_cast<int32_t>(pFileSizeBytes)
			                  : std::numeric_limits<int32_t>::max());
			pOut.NumChannels(nc);
			pOut.SampleRateHz(sr);
			pOut.BytesPerSecond(bytesPerSecond);
			const int bytesPerFrame = std::max(1, static_cast<int>(nc)) * static_cast<int>(bytesPerChan);
			pOut.ByteRate(static_cast<int16_t>(bytesPerFrame));
			pOut.BitsPerSample(bits);
			(void)extractSubtype(static_cast<int>(pSf.format));
		}

		int16_t effectiveBitsPerSample(int pSfFormatFull)
		{
			const int subtype = extractSubtype(pSfFormatFull);
			switch (subtype)
			{
				case SF_FORMAT_PCM_S8:
				case SF_FORMAT_PCM_U8:
					return 8;
				case SF_FORMAT_PCM_16:
					return 16;
				case SF_FORMAT_PCM_24:
					return 24;
				case SF_FORMAT_PCM_32:
					return 32;
				default:
					break;
			}
			const int major = pSfFormatFull & SF_FORMAT_TYPEMASK;
			if (major == SF_FORMAT_MPEG || major == SF_FORMAT_OGG || subtype == SF_FORMAT_VORBIS)
				return 16;
			return 16;
		}

		int64_t maxSampleValueForFormat(int16_t pBitsPerSample)
		{
			if (pBitsPerSample >= 31)
				return 2147483647LL;
			if (pBitsPerSample == 24)
				return 8388607LL;
			if (pBitsPerSample == 16)
				return 32767LL;
			if (pBitsPerSample == 8)
				return 127LL;
			return 2147483647LL;
		}

		std::string formatErrorSuffix(SNDFILE* pSndOrNull)
		{
			if (!pSndOrNull)
				return {};
			const char* err = sf_strerror(pSndOrNull);
			if (err && err[0] != '\0')
				return std::string(": ") + err;
			return {};
		}

		bool formatHasMajorMinor(int pFullFormat, int pMajorMask, int pMinorMask)
		{
			return ((pFullFormat & SF_FORMAT_TYPEMASK) == pMajorMask)
			       && ((pFullFormat & SF_FORMAT_SUBMASK) == pMinorMask);
		}

		std::uintmax_t safeFileSize(const std::string& pPath)
		{
			const std::streamoff sz = fileSizeViaStream(pPath);
			if (sz < 0)
				return 0;
			return static_cast<std::uintmax_t>(sz);
		}

		std::vector<char> peekFilePrefix(const std::string& pPath, std::size_t pMaxLen)
		{
			if (pMaxLen == 0)
				return {};
			std::vector<char> buf(pMaxLen);
			std::ifstream in(pPath, std::ios::binary);
			if (!in)
				return {};
			in.read(buf.data(), static_cast<std::streamsize>(pMaxLen));
			buf.resize(static_cast<std::size_t>(std::max<std::streamsize>(in.gcount(), 0)));
			return buf;
		}
	}
}
