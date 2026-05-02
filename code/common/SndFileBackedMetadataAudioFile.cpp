#include "SndFileBackedMetadataAudioFile.h"
#include "SndFileHelpers.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

using std::string;

namespace EOUtils
{
	namespace
	{
		// sf_read_int / sf_write_int use amplitude-normalized int32 (~±2147483647 FS). The rest of
		// our pipeline (WAV/FLAC/etc.) expects physical PCM in [-max,+max] for BitsPerSample.
		constexpr std::int64_t kSndfileIntFullScale = 2147483647LL;

		std::int64_t normSndIntToPhysical(int pRawNorm, std::int64_t pPhysMagMax)
		{
			if (pPhysMagMax <= 0)
				pPhysMagMax = 1;

			std::int64_t p =
			    static_cast<std::int64_t>(pRawNorm) * pPhysMagMax / kSndfileIntFullScale;
			if (p > pPhysMagMax)
				p = pPhysMagMax;
			else if (p < -pPhysMagMax)
				p = -pPhysMagMax;
			return p;
		}

		int physicalToNormSndInt(std::int64_t pPcmPhys, std::int64_t pPhysMagMax)
		{
			if (pPhysMagMax <= 0)
				pPhysMagMax = 1;

			if (pPcmPhys > pPhysMagMax)
				pPcmPhys = pPhysMagMax;
			else if (pPcmPhys < -pPhysMagMax)
				pPcmPhys = -pPhysMagMax;

			const std::int64_t norm64 = pPcmPhys * kSndfileIntFullScale / pPhysMagMax;

			using Lim = std::numeric_limits<int>;
			if (norm64 >= static_cast<std::int64_t>(Lim::max()))
				return Lim::max();
			if (norm64 <= static_cast<std::int64_t>(Lim::min()))
				return Lim::min();
			return static_cast<int>(norm64);
		}

		bool requiredSfStr(int pKind)
		{
			return pKind == SF_STR_TITLE || pKind == SF_STR_ARTIST || pKind == SF_STR_ALBUM
			       || pKind == SF_STR_COMMENT || pKind == SF_STR_TRACKNUMBER || pKind == SF_STR_GENRE
			       || pKind == SF_STR_DATE;
		}

		void refreshByteSizing(AudioFileInfo& pIo)
		{
			const int16_t ch = std::max<int16_t>(1, pIo.NumChannels());
			const int bits = std::max(1, static_cast<int>(pIo.BitsPerSample()));
			const int bpp = bits / BITS_PER_BYTE;
			pIo.ByteRate(static_cast<int16_t>(ch * bpp));
			pIo.BytesPerSecond(pIo.SampleRateHz() * static_cast<int32_t>(ch) * bpp);
		}
	}

	SndFileBackedMetadataAudioFile::SndFileBackedMetadataAudioFile(const std::string& pFilename)
		: MetadataAudioFile(pFilename)
	{
		mFileLayout.NumChannels(2);
		mFileLayout.SampleRateHz(44100);
		mFileLayout.BitsPerSample(16);
		refreshByteSizing(mFileLayout);
	}

	SndFileBackedMetadataAudioFile::SndFileBackedMetadataAudioFile(const std::string& pFilename, AudioFileModes pFileMode)
		: MetadataAudioFile(pFilename, pFileMode)
	{
		mFileLayout.NumChannels(2);
		mFileLayout.SampleRateHz(44100);
		mFileLayout.BitsPerSample(16);
		refreshByteSizing(mFileLayout);
	}

	SndFileBackedMetadataAudioFile::SndFileBackedMetadataAudioFile(const std::string& pFilename,
	                                                                  const AudioFileInfo& pLayoutHint)
		: MetadataAudioFile(pFilename)
	{
		mFileLayout.copyAudioFileInfo(pLayoutHint);
		refreshByteSizing(mFileLayout);
	}

	SndFileBackedMetadataAudioFile::SndFileBackedMetadataAudioFile(const SndFileBackedMetadataAudioFile& pOther)
		: MetadataAudioFile(static_cast<const MetadataAudioFile&>(pOther)),
		  mFileLayout(pOther.mFileLayout)
	{
	}

	SndFileBackedMetadataAudioFile::SndFileBackedMetadataAudioFile(SndFileBackedMetadataAudioFile&& pOther) noexcept
		: MetadataAudioFile(static_cast<const MetadataAudioFile&>(pOther)),
		  mFileLayout(std::move(pOther.mFileLayout)),
		  mSnd(pOther.mSnd),
		  mSf(pOther.mSf),
		  mFrameBuf(std::move(pOther.mFrameBuf)),
		  mChannelIndexInPump(pOther.mChannelIndexInPump),
		  mWriteAccum(std::move(pOther.mWriteAccum))
	{
		pOther.mSnd = nullptr;
		std::memset(&pOther.mSf, 0, sizeof(pOther.mSf));
		pOther.mChannelIndexInPump = 0;
	}

	SndFileBackedMetadataAudioFile::~SndFileBackedMetadataAudioFile()
	{
		close();
	}

	void SndFileBackedMetadataAudioFile::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		mFileLayout.copyAudioFileInfo(pAudioFileInfo);
		refreshByteSizing(mFileLayout);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::open(AudioFileModes pOpenMode)
	{
		close();
		AudioFileResultType result;

		setFileMode(pOpenMode);
		if (!hasReadMode() && !hasWriteMode())
		{
			result.addError(std::string("SndFileBackedMetadataAudioFile::open() for ") + mFilename
			                + ": Open mode has neither read nor write mode");
			return result;
		}

		std::memset(&mSf, 0, sizeof(mSf));
		resetReadPump();

		const bool reads = hasReadMode();
		const bool writes = hasWriteMode();

		if (reads && writes)
		{
			mSnd = sf_open(mFilename.c_str(), SFM_RDWR, &mSf);
			if (!mSnd)
			{
				const char* er = sf_strerror(nullptr);
				result.addError(string("libsndfile: failed to open ") + mFilename + " RDWR "
				                + (er ? er : ""));
				return result;
			}
			if (!readFormatMatches(static_cast<int>(mSf.format)))
			{
				result.addError(string(formatLabel())
				                + ": file format mismatch for libsndfile read on " + mFilename);
				close();
				return result;
			}
		}
		else if (reads)
		{
			mSf.format = 0;
			mSnd = sf_open(mFilename.c_str(), SFM_READ, &mSf);
			if (!mSnd)
			{
				const char* er = sf_strerror(nullptr);
				result.addError(string("libsndfile: failed to open ") + mFilename + " for read "
				                + (er ? er : ""));
				return result;
			}
			if (!readFormatMatches(static_cast<int>(mSf.format)))
			{
				result.addError(string(formatLabel()) + ": file format mismatch while reading "
				                + mFilename);
				close();
				return result;
			}
		}
		else if (writes)
		{
			if (mFileLayout.NumChannels() <= 0 || mFileLayout.SampleRateHz() <= 0)
			{
				result.addError(string("Invalid channel count or sample rate for writing ")
				                + formatLabel());
				return result;
			}
			mSf.samplerate = mFileLayout.SampleRateHz();
			mSf.channels = mFileLayout.NumChannels();
			mSf.format = newFileSfFormatMask();
			mSf.frames = 0;

			if (!sf_format_check(&mSf))
			{
				result.addError(string("libsndfile: format not supported by this installation for ")
				                + formatLabel() + ": " + mFilename);
				return result;
			}
			mSnd = sf_open(mFilename.c_str(), SFM_WRITE, &mSf);
			if (!mSnd)
			{
				const char* er = sf_strerror(nullptr);
				result.addError(string("libsndfile: failed to open ") + mFilename + " for write "
				                + (er ? er : ""));
				return result;
			}
			configureSndfileEncoderForWrite(mSnd);
		}

		if (reads)
			syncLayoutBytesFromSf();
		else if (writes)
		{
			sndfile_detail::sfInfoToAudioFileInfo(mSf, sndfile_detail::safeFileSize(mFilename),
			                                       mFileLayout);
			mDataSizeBytes = 0;
		}

		return result;
	}

	void SndFileBackedMetadataAudioFile::finalizePartialWriteFrames()
	{
		if (!hasWriteMode() || !mSnd || mWriteAccum.empty())
			return;
		const int ch = std::max(1, static_cast<int>(mFileLayout.NumChannels()));
		while (static_cast<int>(mWriteAccum.size()) < ch)
			mWriteAccum.push_back(0);
		if (static_cast<int>(mWriteAccum.size()) != ch)
			return;

		const sf_count_t nw = sf_writef_int(mSnd, mWriteAccum.data(), 1);
		if (nw == 1)
		{
			bumpDataSizeEstimate();
			mSf.frames++;
		}
		mWriteAccum.clear();
	}

	void SndFileBackedMetadataAudioFile::close()
	{
		finalizePartialWriteFrames();
		if (mSnd)
		{
			sf_close(mSnd);
			mSnd = nullptr;
		}
		std::memset(&mSf, 0, sizeof(mSf));
		mFrameBuf.clear();
		resetReadPump();
		mWriteAccum.clear();
		AudioFile::close();
	}

	bool SndFileBackedMetadataAudioFile::isOpen() const
	{
		return mSnd != nullptr;
	}

	void SndFileBackedMetadataAudioFile::syncLayoutBytesFromSf()
	{
		const std::uintmax_t fz = sndfile_detail::safeFileSize(mFilename);
		sndfile_detail::sfInfoToAudioFileInfo(mSf, fz, mFileLayout);

		const int ch = std::max(1, mSf.channels);
		const int bpp = std::max(1, static_cast<int>(mFileLayout.BitsPerSample()) / BITS_PER_BYTE);
		if (mSf.frames >= 0)
			mDataSizeBytes = static_cast<size_t>(
			    static_cast<sf_count_t>(mSf.frames) * static_cast<sf_count_t>(ch) * bpp);
		else
			mDataSizeBytes = 0;
	}

	void SndFileBackedMetadataAudioFile::resetReadPump()
	{
		mChannelIndexInPump = 0;
		mFrameBuf.clear();
	}

	void SndFileBackedMetadataAudioFile::bumpDataSizeEstimate()
	{
		const int ch = std::max(1, static_cast<int>(mFileLayout.NumChannels()));
		const size_t bpp = std::max<std::size_t>(1u, mFileLayout.BytesPerSample());
		mDataSizeBytes += static_cast<size_t>(ch) * bpp;
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getNextSample_int64(std::int64_t& pAudioSample)
	{
		AudioFileResultType result;
		pAudioSample = 0;
		if (!hasReadMode())
		{
			result.addError(string("Can't read sample: ") + formatLabel() + " file not opened for reading");
			return result;
		}
		if (!mSnd)
		{
			result.addError(string(formatLabel()) + "::getNextSample_int64(): file handle is null");
			return result;
		}

		const unsigned chCount = std::max(1u, static_cast<unsigned>(mSf.channels));
		if (mChannelIndexInPump >= chCount || mFrameBuf.empty())
		{
			mFrameBuf.resize(static_cast<size_t>(chCount));
			const sf_count_t nr = sf_readf_int(mSnd, mFrameBuf.data(), 1);
			if (nr != 1)
			{
				result.addError("End of audio stream or decode error while reading ");
				result.addError(mFilename);
				return result;
			}
			mChannelIndexInPump = 0;
		}

		const std::int64_t physMax =
		    sndfile_detail::maxSampleValueForFormat(mFileLayout.BitsPerSample());
		pAudioSample =
		    normSndIntToPhysical(mFrameBuf[static_cast<size_t>(mChannelIndexInPump)],
		                         physMax);
		mChannelIndexInPump++;
		return result;
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::writeSample_int64(std::int64_t pAudioSample)
	{
		AudioFileResultType result;
		if (!hasWriteMode())
		{
			result.addError(string("Can't write sample: ") + formatLabel() + " file not opened for writing");
			return result;
		}
		if (!mSnd)
		{
			result.addError(string(formatLabel()) + "::writeSample_int64(): file handle is null");
			return result;
		}

		const int ch = std::max(1, static_cast<int>(mFileLayout.NumChannels()));
		const int64_t mx = sndfile_detail::maxSampleValueForFormat(mFileLayout.BitsPerSample());

		const int normInt = physicalToNormSndInt(pAudioSample, mx);

		mWriteAccum.push_back(normInt);
		if (static_cast<int>(mWriteAccum.size()) == ch)
		{
			const sf_count_t nw = sf_writef_int(mSnd, mWriteAccum.data(), 1);
			if (nw != 1)
				result.addError(string("libsndfile: write failed while writing ") + mFilename + sndfile_detail::formatErrorSuffix(mSnd));
			else
			{
				bumpDataSizeEstimate();
				mSf.frames++;
			}
			mWriteAccum.clear();
		}
		return result;
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getHighestSampleValue_int64(std::int64_t& pHighestAudioSample)
	{
		AudioFileResultType result;
		pHighestAudioSample = 0;
		if (!mSnd)
		{
			result.addError(string(formatLabel()) + "::getHighestSampleValue_int64(): file is not open");
			return result;
		}

		goToAudioDataPos();

		const std::int64_t physMax =
		    sndfile_detail::maxSampleValueForFormat(mFileLayout.BitsPerSample());
		std::int64_t best = 0;
		std::vector<int> block(4096u * std::max(1u, static_cast<unsigned>(mSf.channels)));
		for (;;)
		{
			const sf_count_t nf = sf_readf_int(mSnd, block.data(), static_cast<sf_count_t>(block.size() / std::max(1u, static_cast<unsigned>(mSf.channels))));
			if (nf <= 0)
				break;
			const size_t elems = static_cast<size_t>(nf) * std::max(1u, static_cast<unsigned>(mSf.channels));
			for (size_t i = 0; i < elems; ++i)
			{
				const std::int64_t phys = normSndIntToPhysical(block[static_cast<int>(i)], physMax);
				const std::int64_t a = phys >= 0 ? phys : -phys;
				if (a > best)
					best = a;
			}
		}

		pHighestAudioSample = best;
		goToAudioDataPos();
		return result;
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::goToAudioDataPos()
	{
		AudioFileResultType result;
		if (!mSnd)
		{
			result.addError(string(formatLabel()) + "::goToAudioDataPos(): file is not open");
			return result;
		}
		if (sf_seek(mSnd, 0, SEEK_SET) < 0)
			result.addError(string("libsndfile: seek failed on ") + mFilename);
		resetReadPump();
		return result;
	}

	std::size_t SndFileBackedMetadataAudioFile::numSamples() const
	{
		const size_t bps = mFileLayout.BytesPerSample();
		if (bps == 0)
		{
			string msg = string("SndFileBackedMetadataAudioFile::numSamples(): bytes per sample is 0");
			msg += (" for ") + mFilename;
			throw std::logic_error(msg.c_str());
		}
		return mDataSizeBytes / bps;
	}

	std::int64_t SndFileBackedMetadataAudioFile::maxValueForSampleSize() const
	{
		return sndfile_detail::maxSampleValueForFormat(mFileLayout.BitsPerSample());
	}

	void SndFileBackedMetadataAudioFile::seekOutputToSampleNum(std::size_t pSampleNum)
	{
		if (!hasWriteMode() || !mSnd)
			return;

		finalizePartialWriteFrames();
		const int nc = std::max(1, static_cast<int>(mFileLayout.NumChannels()));
		const sf_count_t framed = static_cast<sf_count_t>(pSampleNum / static_cast<std::size_t>(nc));

		const sf_count_t tgt = framed;
		if (sf_seek(mSnd, tgt, SEEK_SET) >= 0)
			mSf.frames = tgt;

		const size_t bpp = std::max<std::size_t>(1u, mFileLayout.BytesPerSample());
		const std::size_t nbytes = static_cast<std::size_t>(tgt) * static_cast<std::size_t>(nc)
		                               * bpp;
		mDataSizeBytes = nbytes;
		resetReadPump();
		mWriteAccum.clear();
	}

	AudioFileInfo SndFileBackedMetadataAudioFile::getAudioFileInfo() const
	{
		return mFileLayout;
	}

	static AudioFileResultType sfGetViaTempRead(const std::string& path, int kind, std::string& val)
	{
		AudioFileResultType res;
		val.clear();

		SF_INFO inf{};
		std::memset(&inf, 0, sizeof(inf));

		SNDFILE* s = sf_open(path.c_str(), SFM_READ, &inf);
		if (!s)
		{
			const char* er = sf_strerror(nullptr);
			res.addError("libsndfile: cannot open metadata for " + path + (er ? string(": ") + er : ""));
			return res;
		}
		const char* got = sf_get_string(s, kind);
		val = got ? got : "";

		if (requiredSfStr(kind) && val.empty())
		{
			std::ostringstream tag;
			tag << "sndfile_metadata: ";
			tag << kind;
			tag << " not present in ";
			tag << path;
			res.addError(tag.str());
		}
		sf_close(s);
		return res;
	}

	static AudioFileResultType sfSetViaRw(const std::string& path, int kind, const std::string& value)
	{
		AudioFileResultType res;

		SF_INFO inf{};
		std::memset(&inf, 0, sizeof(inf));

		SNDFILE* s = sf_open(path.c_str(), SFM_RDWR, &inf);
		if (!s)
		{
			res.addError(string("libsndfile: cannot open RDWR ") + path
			             + "; metadata changes may require reopening write mode explicitly");
			return res;
		}

		const int erc = sf_set_string(s, kind, value.empty() ? nullptr : value.c_str());
		if (erc != 0)
			res.addError(string("libsndfile: sf_set_string failed for ") + path + sndfile_detail::formatErrorSuffix(s));

		sf_close(s);
		return res;
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getSfStringField(int pStrKind, std::string& pOut) const
	{
		AudioFileResultType res;
		pOut.clear();
		if (mSnd != nullptr && hasReadMode())
		{
			const char* v = sf_get_string(mSnd, pStrKind);
			pOut = v ? v : "";
			if (requiredSfStr(pStrKind) && pOut.empty())
			{
				std::ostringstream tag;
				tag << "sndfile_metadata: STR field ";
				tag << pStrKind;
				tag << " not present while reading ";
				tag << mFilename;
				res.addError(tag.str());
			}
			return res;
		}
		return sfGetViaTempRead(mFilename, pStrKind, pOut);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setSfStringField(int pStrKind, const std::string& pVal)
	{
		AudioFileResultType result;
		if (mSnd != nullptr && hasWriteMode())
		{
			const int erc = sf_set_string(mSnd, pStrKind, pVal.empty() ? nullptr : pVal.c_str());
			if (erc != 0)
				result.addError(string("libsndfile: sf_set_string failed on ") + mFilename + sndfile_detail::formatErrorSuffix(mSnd));
			return result;
		}

		return sfSetViaRw(mFilename, pStrKind, pVal);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setTitle(const std::string& pTitle)
	{
		return setSfStringField(SF_STR_TITLE, pTitle);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setArtist(const std::string& pArtist)
	{
		return setSfStringField(SF_STR_ARTIST, pArtist);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setAlbum(const std::string& pAlbum)
	{
		return setSfStringField(SF_STR_ALBUM, pAlbum);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setGenre(const std::string& pGenre)
	{
		return setSfStringField(SF_STR_GENRE, pGenre);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setYear(const std::string& pYear)
	{
		return setSfStringField(SF_STR_DATE, pYear);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setComment(const std::string& pComment)
	{
		return setSfStringField(SF_STR_COMMENT, pComment);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::setTrackNumber(std::uint32_t pTrackNumber,
	                                                                   std::uint32_t pTotalTracks)
	{
		std::ostringstream oss;
		oss << pTrackNumber;
		if (pTotalTracks != 0u)
			oss << '/' << pTotalTracks;
		return setSfStringField(SF_STR_TRACKNUMBER, oss.str());
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getTitle(std::string& pTitle) const
	{
		return getSfStringField(SF_STR_TITLE, pTitle);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getArtist(std::string& pArtist) const
	{
		return getSfStringField(SF_STR_ARTIST, pArtist);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getAlbum(std::string& pAlbum) const
	{
		return getSfStringField(SF_STR_ALBUM, pAlbum);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getGenre(std::string& pGenre) const
	{
		return getSfStringField(SF_STR_GENRE, pGenre);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getYear(std::string& pYear) const
	{
		return getSfStringField(SF_STR_DATE, pYear);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getComment(std::string& pComment) const
	{
		return getSfStringField(SF_STR_COMMENT, pComment);
	}

	AudioFileResultType SndFileBackedMetadataAudioFile::getTrackNumber(std::uint32_t& pTrackNumber,
	                                                                   std::uint32_t& pTotalTracks) const
	{
		pTrackNumber = 0;
		pTotalTracks = 0;
		string value;
		const AudioFileResultType resultWithVal = getSfStringField(SF_STR_TRACKNUMBER, value);
		if (!resultWithVal)
			return resultWithVal;

		try
		{
			size_t slash = value.find('/');
			if (slash != string::npos)
			{
				pTrackNumber = static_cast<std::uint32_t>(std::stoul(value.substr(0, slash)));
				pTotalTracks = static_cast<std::uint32_t>(std::stoul(value.substr(slash + 1)));
			}
			else
				pTrackNumber = static_cast<std::uint32_t>(std::stoul(value));
		}
		catch (...)
		{
			AudioFileResultType bad;
			bad.addError("Invalid TRACKNUMBER metadata: " + value);
			return bad;
		}
		return AudioFileResultType();
	}
}
