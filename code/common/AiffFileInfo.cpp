#include "AiffFileInfo.h"
#include "SndFileHelpers.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <sndfile.h>
#include <string>
#include <vector>

using std::fstream;
using std::string;

namespace EOUtils
{
	namespace
	{
		bool pcmSubtype(int subtype)
		{
			switch (subtype)
			{
				case SF_FORMAT_PCM_S8:
				case SF_FORMAT_PCM_16:
				case SF_FORMAT_PCM_24:
				case SF_FORMAT_PCM_32:
					return true;
				default:
					return false;
			}
		}
	}

	AiffFileInfo::AiffFileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond,
	                           int16_t pByteRate, int16_t pBitsPerSample)
		: AudioFileInfo(pNumChannels, pSampleRateHz, pBytesPerSecond, pByteRate, pBitsPerSample)
	{
	}

	AiffFileInfo::AiffFileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo)
	{
	}

	AudioFileResultType AiffFileInfo::read(std::fstream& /*pInFStream*/)
	{
		AudioFileResultType result;
		result.addError("AiffFileInfo::read(fstream&) is not used; call read(filename) instead.");
		return result;
	}

	AudioFileResultType AiffFileInfo::read(const char* pFilename)
	{
		AudioFileResultType result;
		if (pFilename == nullptr || pFilename[0] == '\0')
		{
			result.addError("AiffFileInfo::read(): empty filename");
			return result;
		}

		SF_INFO inf{};
		std::memset(&inf, 0, sizeof(inf));

		SNDFILE* s = sf_open(pFilename, SFM_READ, &inf);
		if (!s)
		{
			const char* er = sf_strerror(nullptr);
			result.addError(string("AiffFileInfo::read(): cannot open ") + pFilename + (er ? string(": ") + er : ""));
			return result;
		}

		const int full = static_cast<int>(inf.format);
		if ((full & SF_FORMAT_TYPEMASK) != SF_FORMAT_AIFF || !pcmSubtype(full & SF_FORMAT_SUBMASK))
		{
			sf_close(s);
			result.addError(string("Not a PCM AIFF file according to libsndfile: ") + string(pFilename));
			return result;
		}

		sndfile_detail::sfInfoToAudioFileInfo(inf, sndfile_detail::safeFileSize(string(pFilename)), *this);
		sf_close(s);
		return result;
	}

	AudioFileResultType AiffFileInfo::write(std::fstream& /*pOutFStream*/)
	{
		AudioFileResultType result;
		result.addError("AiffFileInfo::write(fstream&) is unused; PCM headers go through AIFF via AiffFile + libsndfile.");
		return result;
	}

	bool AiffFileInfo::matchesExtension(const char* pFilename)
	{
		if (!pFilename)
			return false;
		string fn(pFilename);
		const auto dot = fn.find_last_of('.');
		if (dot != string::npos && dot + 1 < fn.size())
		{
			string ext = fn.substr(dot + 1);
			for (char& ch : ext)
				ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			return ext == "AIFF" || ext == "AIF" || ext == "AIFC";
		}
		return false;
	}

	bool AiffFileInfo::sniffFileHeader(const char* pFilename)
	{
		if (!pFilename)
			return false;
		std::vector<char> pfx = sndfile_detail::peekFilePrefix(string(pFilename), 16);
		if (pfx.size() < 12)
			return false;
		if (!(pfx[0] == 'F' && pfx[1] == 'O' && pfx[2] == 'R' && pfx[3] == 'M'))
			return false;

		auto form = [&](size_t idx) -> bool {
			return idx + 3 < pfx.size()
			       && (((pfx[idx] == 'A' || pfx[idx] == 'a') && (pfx[idx + 1] == 'I' || pfx[idx + 1] == 'i')
			       && (pfx[idx + 2] == 'F' || pfx[idx + 2] == 'f') && (pfx[idx + 3] == 'F' || pfx[idx + 3] == 'f'))
			               || ((pfx[idx] == 'A' || pfx[idx] == 'a') && (pfx[idx + 1] == 'I' || pfx[idx + 1] == 'i')
			                       && (pfx[idx + 2] == 'F' || pfx[idx + 2] == 'f')
			                       && (pfx[idx + 3] == 'C' || pfx[idx + 3] == 'c')));
		};
		return form(8);
	}
}
