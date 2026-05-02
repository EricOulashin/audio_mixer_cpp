#include "OggFileInfo.h"
#include "SndFileHelpers.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <sndfile.h>

using std::fstream;
using std::string;

namespace EOUtils
{
	OggFileInfo::OggFileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond,
	                         int16_t pByteRate, int16_t pBitsPerSample)
		: AudioFileInfo(pNumChannels, pSampleRateHz, pBytesPerSecond, pByteRate, pBitsPerSample)
	{
	}

	OggFileInfo::OggFileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo)
	{
	}

	AudioFileResultType OggFileInfo::read(std::fstream& /*pInFStream*/)
	{
		AudioFileResultType result;
		result.addError("OggFileInfo::read(fstream&) is not used; call read(filename) instead.");
		return result;
	}

	AudioFileResultType OggFileInfo::read(const char* pFilename)
	{
		AudioFileResultType result;
		if (pFilename == nullptr || pFilename[0] == '\0')
		{
			result.addError("OggFileInfo::read(): empty filename");
			return result;
		}

		SF_INFO inf{};
		std::memset(&inf, 0, sizeof(inf));

		SNDFILE* s = sf_open(pFilename, SFM_READ, &inf);
		if (!s)
		{
			const char* er = sf_strerror(nullptr);
			result.addError(string("OggFileInfo::read(): cannot open ") + pFilename + (er ? string(": ") + er : ""));
			return result;
		}

		if (!sndfile_detail::formatHasMajorMinor(static_cast<int>(inf.format), SF_FORMAT_OGG, SF_FORMAT_VORBIS))
		{
			sf_close(s);
			result.addError(string("Not an Ogg Vorbis file according to libsndfile: ") + string(pFilename));
			return result;
		}

		sndfile_detail::sfInfoToAudioFileInfo(inf, sndfile_detail::safeFileSize(string(pFilename)), *this);
		sf_close(s);
		return result;
	}

	AudioFileResultType OggFileInfo::write(std::fstream& /*pOutFStream*/)
	{
		AudioFileResultType result;
		result.addError("OggFileInfo::write(fstream&) is unused; Ogg/Vorbis streams are encoded via OggFile + libsndfile.");
		return result;
	}

	bool OggFileInfo::matchesExtension(const char* pFilename)
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
			return ext == "OGG" || ext == "OGA";
		}
		return false;
	}

	bool OggFileInfo::sniffFileHeader(const char* pFilename)
	{
		if (!pFilename)
			return false;
		std::vector<char> pfx = sndfile_detail::peekFilePrefix(string(pFilename), 8);
		if (pfx.size() < 4)
			return false;
		return std::memcmp(pfx.data(), "OggS", 4) == 0;
	}
}
