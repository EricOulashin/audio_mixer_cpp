#include "MP3FileInfo.h"
#include "SndFileHelpers.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <vector>
#include <sndfile.h>
#include <string>

using std::fstream;
using std::string;

namespace EOUtils
{
	MP3FileInfo::MP3FileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond,
	                         int16_t pByteRate, int16_t pBitsPerSample)
		: AudioFileInfo(pNumChannels, pSampleRateHz, pBytesPerSecond, pByteRate, pBitsPerSample)
	{
	}

	MP3FileInfo::MP3FileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo)
	{
	}

	AudioFileResultType MP3FileInfo::read(std::fstream& /*pInFStream*/)
	{
		AudioFileResultType result;
		result.addError("MP3FileInfo::read(fstream&) is not used; call read(filename) instead.");
		return result;
	}

	AudioFileResultType MP3FileInfo::read(const char* pFilename)
	{
		AudioFileResultType result;
		if (pFilename == nullptr || pFilename[0] == '\0')
		{
			result.addError("MP3FileInfo::read(): empty filename");
			return result;
		}

		SF_INFO inf{};
		std::memset(&inf, 0, sizeof(inf));

		SNDFILE* s = sf_open(pFilename, SFM_READ, &inf);
		if (!s)
		{
			const char* er = sf_strerror(nullptr);
			result.addError(string("MP3FileInfo::read(): cannot open ") + pFilename + (er ? string(": ") + er : ""));
			return result;
		}

		if (!sndfile_detail::formatHasMajorMinor(static_cast<int>(inf.format), SF_FORMAT_MPEG,
		                                         SF_FORMAT_MPEG_LAYER_III))
		{
			sf_close(s);
			result.addError(string("Not an MPEG Layer III MP3 according to libsndfile: ")
			                + string(pFilename));
			return result;
		}

		sndfile_detail::sfInfoToAudioFileInfo(inf, sndfile_detail::safeFileSize(string(pFilename)), *this);
		sf_close(s);
		return result;
	}

	AudioFileResultType MP3FileInfo::write(std::fstream& /*pOutFStream*/)
	{
		AudioFileResultType result;
		result.addError("MP3FileInfo::write(fstream&) is unused; MPEG data is encoded via MP3File + libsndfile.");
		return result;
	}

	bool MP3FileInfo::matchesExtension(const char* pFilename)
	{
		if (!pFilename)
			return false;
		string ext;
		string fn(pFilename);
		const auto dot = fn.find_last_of('.');
		if (dot != string::npos && dot + 1 < fn.size())
			ext = fn.substr(dot + 1);
		for (char& ch : ext)
			ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
		return ext == "MP3";
	}

	bool MP3FileInfo::sniffFileHeader(const char* pFilename)
	{
		if (!pFilename)
			return false;
		std::vector<char> pfx = sndfile_detail::peekFilePrefix(string(pFilename), 16);
		if (pfx.size() < 3)
			return false;
		const unsigned char* u = reinterpret_cast<const unsigned char*>(pfx.data());

		if ((u[0] == 'I' && u[1] == 'D' && u[2] == '3')
		    || (((u[0] & 0xFFu) == 0xFFu) && ((u[1] & 0xE0u) == 0xE0u)))
			return true;
		return false;
	}
}
