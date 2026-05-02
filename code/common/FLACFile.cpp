#include "FLACFile.h"
#include "utilFunctions.h"

#include <algorithm>
#include <utility>
#include <cstring>
#include <sstream>
#include <stdexcept>

extern "C" {
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
}

using std::logic_error;
using std::string;
using std::ios_base;
using std::ostringstream;

namespace EOUtils
{
	namespace
	{
		FLAC__StreamDecoderReadStatus decoderReadCb(const FLAC__StreamDecoder* /*decoder*/, FLAC__byte buffer[], size_t* bytes, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			std::fstream* stream = data->stream;
			if (stream && *bytes > 0)
			{
				stream->read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(*bytes));
				*bytes = static_cast<size_t>(stream->gcount());
				if (stream->eof() && *bytes == 0)
					return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
				if (stream->fail() && !stream->eof())
					return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
			}
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}

		FLAC__StreamDecoderSeekStatus decoderSeekCb(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64 absolute_byte_offset, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			if (data->stream && data->stream->seekg(static_cast<std::streamoff>(absolute_byte_offset), std::ios_base::beg))
				return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
			return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
		}

		FLAC__StreamDecoderTellStatus decoderTellCb(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* absolute_byte_offset, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			if (data->stream)
			{
				std::streampos pos = data->stream->tellg();
				if (pos >= 0)
				{
					*absolute_byte_offset = static_cast<FLAC__uint64>(pos);
					return FLAC__STREAM_DECODER_TELL_STATUS_OK;
				}
			}
			return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
		}

		FLAC__StreamDecoderLengthStatus decoderLengthCb(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* stream_length, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			if (data->stream)
			{
				std::streampos curr = data->stream->tellg();
				data->stream->seekg(0, std::ios_base::end);
				std::streampos end = data->stream->tellg();
				data->stream->seekg(curr, std::ios_base::beg);
				if (end >= 0)
				{
					*stream_length = static_cast<FLAC__uint64>(end);
					return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
				}
			}
			return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
		}

		FLAC__bool decoderEofCb(const FLAC__StreamDecoder* /*decoder*/, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			return data->stream ? data->stream->eof() : true;
		}

		FLAC__StreamDecoderWriteStatus decoderWriteCb(const FLAC__StreamDecoder* /*decoder*/, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			const unsigned channels = frame->header.channels;
			const unsigned blockSize = frame->header.blocksize;

			// Store channels in separate vectors; we output interleaved (ch0_s0, ch1_s0, ch0_s1, ch1_s1, ...)
			data->readBuffer->resize(channels);
			for (unsigned ch = 0; ch < channels; ++ch)
			{
				(*data->readBuffer)[ch].assign(buffer[ch], buffer[ch] + blockSize);
			}
			*data->readChannel = 0;
			*data->readSample = 0;
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}

		void decoderMetadataCb(const FLAC__StreamDecoder* /*decoder*/, const FLAC__StreamMetadata* metadata, void* client_data)
		{
			if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO)
				return;

			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			FLACFileInfo* info = data->fileInfo;
			if (!info)
				return;

			info->NumChannels(static_cast<int16_t>(metadata->data.stream_info.channels));
			info->SampleRateHz(static_cast<int32_t>(metadata->data.stream_info.sample_rate));
			info->BitsPerSample(static_cast<int16_t>(metadata->data.stream_info.bits_per_sample));
			info->ByteRate(static_cast<int16_t>(info->NumChannels() * (info->BitsPerSample() / BITS_PER_BYTE)));
			info->BytesPerSecond(info->SampleRateHz() * info->ByteRate());
		}

		void decoderErrorCb(const FLAC__StreamDecoder* /*decoder*/, FLAC__StreamDecoderErrorStatus status, void* client_data)
		{
			FLACFile::DecoderClientData* data = static_cast<FLACFile::DecoderClientData*>(client_data);
			if (data->result)
			{
				const char* msg = FLAC__StreamDecoderErrorStatusString[status];
				data->result->addError(std::string("FLAC decoder: ") + (msg ? msg : "unknown error"));
			}
		}

		void encoderProgressCb(const FLAC__StreamEncoder* /*encoder*/, FLAC__uint64 /*bytes_written*/, FLAC__uint64 /*samples_written*/, uint32_t /*frames_written*/, uint32_t /*total_frames_estimate*/, void* /*client_data*/)
		{
			// Progress callback; can be used for optional logging
		}
	}

	FLACFile::FLACFile(const string& pFilename)
		: MetadataAudioFile(pFilename),
		  mReadBufferChannel(0),
		  mReadBufferSample(0),
		  mDecodePosition(0)
	{
	}

	FLACFile::FLACFile(const string& pFilenam, uint32_t pCompressionLevel)
		: MetadataAudioFile(pFilenam),
		  mReadBufferChannel(0),
		  mReadBufferSample(0),
		  mDecodePosition(0)
	{
		mFLACFileInfo.CompressionLevel(pCompressionLevel);
	}

	FLACFile::FLACFile(const string& pFilename, const FLACFileInfo& pFLACFileInfo)
		: MetadataAudioFile(pFilename),
		  mFLACFileInfo(pFLACFileInfo),
		  mReadBufferChannel(0),
		  mReadBufferSample(0),
		  mDecodePosition(0)
	{
	}

	FLACFile::FLACFile(const string& pFilename, AudioFileModes pFileMode)
		: MetadataAudioFile(pFilename, pFileMode),
		  mReadBufferChannel(0),
		  mReadBufferSample(0),
		  mDecodePosition(0)
	{
	}

	FLACFile::FLACFile(const FLACFile& pFLACFile)
		: MetadataAudioFile(pFLACFile),
		  mFLACFileInfo(pFLACFile.mFLACFileInfo),
		  mReadBufferChannel(0),
		  mReadBufferSample(0),
		  mDecodePosition(pFLACFile.mDecodePosition)
	{
	}

	FLACFile::FLACFile(FLACFile&& pFLACFile) noexcept
		: MetadataAudioFile(pFLACFile),
		  mFLACFileInfo(std::move(pFLACFile.mFLACFileInfo)),
		  mReadBuffer(std::move(pFLACFile.mReadBuffer)),
		  mReadBufferChannel(pFLACFile.mReadBufferChannel),
		  mReadBufferSample(pFLACFile.mReadBufferSample),
		  mWriteBuffer(std::move(pFLACFile.mWriteBuffer)),
		  mDecodePosition(pFLACFile.mDecodePosition)
	{
		pFLACFile.close();
		pFLACFile.mReadBufferChannel = 0;
		pFLACFile.mReadBufferSample = 0;
		pFLACFile.mDecodePosition = 0;
	}

	FLACFile::~FLACFile()
	{
		close();
	}

	bool FLACFile::BitrateIsAdjustable() const
	{
		return false;
	}

	void FLACFile::setAudioFileInfo(const AudioFileInfo& pAudioFileInfo)
	{
		mFLACFileInfo.copyAudioFileInfo(pAudioFileInfo);
	}

	uint32_t FLACFile::CompressionLevel() const
	{
		return mFLACFileInfo.CompressionLevel();
	}

	void FLACFile::CompressionLevel(uint32_t pCompressionLevel)
	{
		mFLACFileInfo.CompressionLevel(pCompressionLevel);
	}

	AudioFileResultType FLACFile::open(AudioFileModes pOpenMode)
	{
		if (mDecoder)
		{
			FLAC__stream_decoder_finish(static_cast<FLAC__StreamDecoder*>(mDecoder));
			FLAC__stream_decoder_delete(static_cast<FLAC__StreamDecoder*>(mDecoder));
			mDecoder = nullptr;
		}
		if (mEncoder)
		{
			FLAC__stream_encoder_finish(static_cast<FLAC__StreamEncoder*>(mEncoder));
			FLAC__stream_encoder_delete(static_cast<FLAC__StreamEncoder*>(mEncoder));
			mEncoder = nullptr;
		}
		if (mFileStream.is_open())
			mFileStream.close();

		mDataSizeBytes = 0;
		mReadBuffer.clear();
		mReadBufferChannel = 0;
		mReadBufferSample = 0;
		mWriteBuffer.clear();
		mDecodePosition = 0;

		AudioFileResultType result = AudioFile::open(pOpenMode);
		if (!result)
			return result;

		const bool readMode = hasReadMode();
		const bool writeMode = hasWriteMode();

		if (readMode)
		{
			mFileStream.seekg(0, std::ios_base::beg);
			FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
			if (!decoder)
			{
				mFileStream.close();
				result.addError("FLACFile::open(): Failed to create FLAC decoder");
				return result;
			}

			mDecoderClientData.stream = &mFileStream;
			mDecoderClientData.fileInfo = &mFLACFileInfo;
			mDecoderClientData.readBuffer = &mReadBuffer;
			mDecoderClientData.readChannel = &mReadBufferChannel;
			mDecoderClientData.readSample = &mReadBufferSample;
			mDecoderClientData.result = &result;

			FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(
				decoder,
				decoderReadCb,
				decoderSeekCb,
				decoderTellCb,
				decoderLengthCb,
				decoderEofCb,
				decoderWriteCb,
				decoderMetadataCb,
				decoderErrorCb,
				&mDecoderClientData
			);

			if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			{
				result.addError(std::string("FLACFile::open(): Decoder init failed: ") +
					(FLAC__StreamDecoderInitStatusString[initStatus] ? FLAC__StreamDecoderInitStatusString[initStatus] : "unknown"));
				FLAC__stream_decoder_delete(decoder);
				mFileStream.close();
				return result;
			}

			FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_metadata(decoder);
			if (!ok && result.numErrors() == 0)
				result.addError("FLACFile::open(): Failed to read FLAC metadata");

			if (!result)
			{
				FLAC__stream_decoder_finish(decoder);
				FLAC__stream_decoder_delete(decoder);
				mFileStream.close();
				return result;
			}

			mDecoder = decoder;
		}
		else if (writeMode)
		{
			if (mFLACFileInfo.BitsPerSample() <= 0 || mFLACFileInfo.NumChannels() <= 0 || mFLACFileInfo.SampleRateHz() <= 0)
			{
				mFileStream.close();
				result.addError("FLACFile::open(): Invalid audio parameters for writing (channels, sample rate, or bits per sample <= 0)");
				return result;
			}

			// Use init_file so FLAC manages the file (seek/tell for STREAMINFO update); close our stream first
			mFileStream.close();

			FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
			if (!encoder)
			{
				result.addError("FLACFile::open(): Failed to create FLAC encoder");
				return result;
			}

			FLAC__bool ok = true;
			ok = ok && FLAC__stream_encoder_set_channels(encoder, static_cast<uint32_t>(mFLACFileInfo.NumChannels()));
			ok = ok && FLAC__stream_encoder_set_bits_per_sample(encoder, static_cast<uint32_t>(mFLACFileInfo.BitsPerSample()));
			ok = ok && FLAC__stream_encoder_set_sample_rate(encoder, static_cast<uint32_t>(mFLACFileInfo.SampleRateHz()));
			ok = ok && FLAC__stream_encoder_set_compression_level(encoder, mFLACFileInfo.CompressionLevel());

			// Set total samples estimate if available (required for correct numSamples when file is reopened)
			if (hasMetadata("totalSamples"))
			{
				const size_t totalInterleaved = getMetadataAs<size_t>("totalSamples");
				const uint32_t channels = static_cast<uint32_t>(mFLACFileInfo.NumChannels());
				const FLAC__uint64 samplesPerChannel = (channels > 0) ? (totalInterleaved / channels) : 0;
				ok = ok && FLAC__stream_encoder_set_total_samples_estimate(encoder, samplesPerChannel);
			}

			if (!ok)
			{
				FLAC__stream_encoder_delete(encoder);
				result.addError("FLACFile::open(): Failed to set encoder parameters");
				return result;
			}

			FLAC__StreamEncoderInitStatus initStatus = FLAC__stream_encoder_init_file(
				encoder,
				mFilename.c_str(),
				encoderProgressCb,
				nullptr   // client_data
			);

			if (initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
			{
				result.addError(std::string("FLACFile::open(): Encoder init failed: ") +
					(FLAC__StreamEncoderInitStatusString[initStatus] ? FLAC__StreamEncoderInitStatusString[initStatus] : "unknown"));
				FLAC__stream_encoder_delete(encoder);
				return result;
			}

			mEncoder = encoder;
		}

		return result;
	}

	bool FLACFile::isOpen() const
	{
		// When using init_file for write, mFileStream is not used; encoder manages the file
		return mFileStream.is_open() || (mEncoder != nullptr) || (mDecoder != nullptr);
		/*
		bool isOpen = false;
		if (AudioFile::isOpen())
		{
			if (hasReadMode())
				isOpen = isOpen && (mDecoder != nullptr);
			if (hasWriteMode())
				isOpen = isOpen && (mEncoder != nullptr);
		}
		return isOpen;
		*/
	}

	void FLACFile::close()
	{
		if (mEncoder)
		{
			FLAC__StreamEncoder* encoder = static_cast<FLAC__StreamEncoder*>(mEncoder);
			// Flush any remaining samples in the write buffer
			if (!mWriteBuffer.empty())
			{
				const uint32_t channels = static_cast<uint32_t>(mFLACFileInfo.NumChannels());
				FLAC__stream_encoder_process_interleaved(encoder, mWriteBuffer.data(), static_cast<uint32_t>(mWriteBuffer.size() / channels));
				mWriteBuffer.clear();
			}
			FLAC__stream_encoder_finish(encoder);
			FLAC__stream_encoder_delete(encoder);
			mEncoder = nullptr;
		}

		if (mDecoder)
		{
			FLAC__StreamDecoder* decoder = static_cast<FLAC__StreamDecoder*>(mDecoder);
			FLAC__stream_decoder_finish(decoder);
			FLAC__stream_decoder_delete(decoder);
			mDecoder = nullptr;
		}

		if (mFileStream.is_open())
			mFileStream.close();
	}

	const FLACFileInfo& FLACFile::getFileInfo() const
	{
		return mFLACFileInfo;
	}

	AudioFileResultType FLACFile::getNextSample_int64(int64_t& pAudioSample)
	{
		AudioFileResultType result;
		pAudioSample = 0;

		if (!hasReadMode())
		{
			result.addError("FLACFile::getNextSample_int64(): Not in read mode");
			return result;
		}

		if (!mDecoder)
		{
			result.addError("FLACFile::getNextSample_int64(): Decoder not initialized");
			return result;
		}

		FLAC__StreamDecoder* decoder = static_cast<FLAC__StreamDecoder*>(mDecoder);

		while (mReadBuffer.empty() || mReadBufferChannel >= mReadBuffer.size() || mReadBufferSample >= mReadBuffer[mReadBufferChannel].size())
		{
			FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED)
			{
				result.addError("FLACFile::getNextSample_int64(): At end of stream");
				return result;
			}

			if (!FLAC__stream_decoder_process_single(decoder))
			{
				state = FLAC__stream_decoder_get_state(decoder);
				if (state == FLAC__STREAM_DECODER_END_OF_STREAM)
				{
					result.addError("FLACFile::getNextSample_int64(): At end of stream");
					return result;
				}
				if (result.numErrors() == 0)
					result.addError("FLACFile::getNextSample_int64(): Decode error");
				return result;
			}

			state = FLAC__stream_decoder_get_state(decoder);
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM)
			{
				result.addError("FLACFile::getNextSample_int64(): At end of stream");
				return result;
			}
			// process_single may decode a metadata block (write callback not called); keep processing until we get audio
		}

		const int16_t bits = mFLACFileInfo.BitsPerSample();
		int32_t sample = mReadBuffer[mReadBufferChannel][mReadBufferSample];

		if (bits == 16)
		{
			sample = std::max(-32768, std::min(32767, sample));
		}
		else if (bits == 24)
		{
			sample = sample >> 8;
			sample = std::max(-8388608, std::min(8388607, sample));
		}

		pAudioSample = static_cast<int64_t>(sample);
		// Advance to next sample in interleaved order (ch0_s0, ch1_s0, ch0_s1, ch1_s1, ...)
		++mReadBufferChannel;
		if (mReadBufferChannel >= mReadBuffer.size())
		{
			mReadBufferChannel = 0;
			++mReadBufferSample;
		}

		return result;
	}

	AudioFileResultType FLACFile::writeSample_int64(int64_t pAudioSample)
	{
		AudioFileResultType result;

		if (!hasWriteMode())
		{
			result.addError("FLACFile::writeSample_int64(): Not in write mode");
			return result;
		}

		if (!mEncoder)
		{
			result.addError("FLACFile::writeSample_int64(): Encoder not initialized");
			return result;
		}

		const int16_t bits = mFLACFileInfo.BitsPerSample();
		int32_t sample;
		if (bits == 8)
			sample = static_cast<int32_t>(static_cast<uint8_t>(pAudioSample & 0xFF));
		else if (bits == 16)
			sample = static_cast<int32_t>(static_cast<int16_t>(pAudioSample & 0xFFFF));
		else if (bits == 24 || bits == 32)
			sample = static_cast<int32_t>(pAudioSample);
		else
		{
			ostringstream oss;
			oss << "FLACFile::writeSample_int64(): Unsupported bits per sample: " << bits;
			result.addError(oss.str());
			return result;
		}

		mWriteBuffer.push_back(sample);

		const size_t samplesPerBlock = 4096;
		const size_t numChannels = static_cast<size_t>(mFLACFileInfo.NumChannels());
		if (mWriteBuffer.size() >= samplesPerBlock * numChannels)
		{
			FLAC__StreamEncoder* encoder = static_cast<FLAC__StreamEncoder*>(mEncoder);
			const uint32_t samplesToProcess = static_cast<uint32_t>(samplesPerBlock);
			if (!FLAC__stream_encoder_process_interleaved(encoder, mWriteBuffer.data(), samplesToProcess))
			{
				FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(encoder);
				result.addError(std::string("FLACFile::writeSample_int64(): Encode error: ") +
					(FLAC__StreamEncoderStateString[state] ? FLAC__StreamEncoderStateString[state] : "unknown"));
				return result;
			}
			mWriteBuffer.erase(mWriteBuffer.begin(), mWriteBuffer.begin() + static_cast<std::ptrdiff_t>(samplesPerBlock * numChannels));
		}

		return result;
	}

	AudioFileResultType FLACFile::getHighestSampleValue_int64(int64_t& pHighestAudioSample)
	{
		AudioFileResultType result;
		pHighestAudioSample = 0;

		if (!mDecoder)
		{
			result.addError("FLACFile::getHighestSampleValue_int64(): Decoder not initialized");
			return result;
		}

		AudioFileResultType seekResult = goToAudioDataPos();
		if (!seekResult)
			return seekResult;

		int64_t highest = 0;
		int64_t sample = 0;
		const size_t totalSamples = numSamples();
		for (size_t i = 0; i < totalSamples && result; ++i)
		{
			result = getNextSample_int64(sample);
			if (result)
			{
				int64_t absSample = (sample < 0) ? -sample : sample;
				if (absSample > highest)
					highest = absSample;
			}
		}

		if (result)
			pHighestAudioSample = highest;

		goToAudioDataPos();
		return result;
	}

	AudioFileResultType FLACFile::goToAudioDataPos()
	{
		AudioFileResultType result;

		if (!mDecoder)
		{
			result.addError("FLACFile::goToAudioDataPos(): Decoder not initialized");
			return result;
		}

		FLAC__StreamDecoder* decoder = static_cast<FLAC__StreamDecoder*>(mDecoder);
		mReadBuffer.clear();
		mReadBufferChannel = 0;
		mReadBufferSample = 0;
		mDecodePosition = 0;

		if (!FLAC__stream_decoder_seek_absolute(decoder, 0))
		{
			FLAC__stream_decoder_flush(decoder);
		}

		return result;
	}

	size_t FLACFile::numSamples() const
	{
		if (mFLACFileInfo.BytesPerSample() == 0)
		{
			string msg = "FLACFile::numSamples(): Bytes per sample is 0 for " + mFilename;
			throw logic_error(msg.c_str());
		}
		FLAC__uint64 samplesPerChannel = 0;
		if (mDecoder)
		{
			samplesPerChannel = FLAC__stream_decoder_get_total_samples(static_cast<const FLAC__StreamDecoder*>(mDecoder));
		}
		const uint32_t channels = static_cast<uint32_t>(mFLACFileInfo.NumChannels());
		return static_cast<size_t>(samplesPerChannel * channels);
	}

	int64_t FLACFile::maxValueForSampleSize() const
	{
		switch (mFLACFileInfo.BitsPerSample())
		{
			case 8:
				return static_cast<int64_t>(maxValue<uint8_t>());
			case 16:
				return static_cast<int64_t>(maxValue<int16_t>());
			case 24:
				return (1 << 23) - 1;
			case 32:
				return static_cast<int64_t>(maxValue<int32_t>());
			default:
				return static_cast<int64_t>(maxValue<int16_t>());
		}
	}

	void FLACFile::seekOutputToSampleNum(size_t pSampleNum)
	{
		if (!mEncoder)
			return;

		const size_t numChannels = static_cast<size_t>(mFLACFileInfo.NumChannels());
		const size_t currentSamples = mWriteBuffer.size() / numChannels;
		if (pSampleNum > currentSamples)
		{
			const size_t padCount = (pSampleNum - currentSamples) * numChannels;
			for (size_t i = 0; i < padCount; ++i)
				mWriteBuffer.push_back(0);
		}
	}

	AudioFileInfo FLACFile::getAudioFileInfo() const
	{
		return mFLACFileInfo;
	}

	// Private helpers for FLAC Vorbis Comment metadata

	AudioFileResultType FLACFile::setVorbisComment(const string& pTagName, const string& pValue)
	{
		AudioFileResultType result;

		FLAC__Metadata_Chain* chain = FLAC__metadata_chain_new();
		if (!chain)
		{
			result.addError("FLACFile::setVorbisComment(): Failed to create metadata chain");
			return result;
		}

		if (!FLAC__metadata_chain_read(chain, mFilename.c_str()))
		{
			result.addError("FLACFile::setVorbisComment(): Failed to read metadata chain from " + mFilename);
			FLAC__metadata_chain_delete(chain);
			return result;
		}

		FLAC__Metadata_Iterator* iter = FLAC__metadata_iterator_new();
		if (!iter)
		{
			result.addError("FLACFile::setVorbisComment(): Failed to create metadata iterator");
			FLAC__metadata_chain_delete(chain);
			return result;
		}

		FLAC__metadata_iterator_init(iter, chain);

		// Find existing VORBIS_COMMENT block
		FLAC__StreamMetadata* block = nullptr;
		do
		{
			block = FLAC__metadata_iterator_get_block(iter);
			if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
				break;
			block = nullptr;
		} while (FLAC__metadata_iterator_next(iter));

		if (!block)
		{
			// Create a new VORBIS_COMMENT block
			block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
			if (!block || !FLAC__metadata_iterator_insert_block_after(iter, block))
			{
				result.addError("FLACFile::setVorbisComment(): Failed to create Vorbis Comment block");
				if (block)
					FLAC__metadata_object_delete(block);
				FLAC__metadata_iterator_delete(iter);
				FLAC__metadata_chain_delete(chain);
				return result;
			}
		}

		// Build the entry string: "TAGNAME=value"
		string entryStr = pTagName + "=" + pValue;
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		entry.length = static_cast<FLAC__uint32>(entryStr.size());
		entry.entry = reinterpret_cast<FLAC__byte*>(const_cast<char*>(entryStr.c_str()));

		// Update existing entry or append a new one
		int entryIdx = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, pTagName.c_str());
		if (entryIdx >= 0)
		{
			if (!FLAC__metadata_object_vorbiscomment_set_comment(block, static_cast<uint32_t>(entryIdx), entry, /*copy=*/true))
			{
				result.addError("FLACFile::setVorbisComment(): Failed to update Vorbis Comment entry for " + pTagName);
				FLAC__metadata_iterator_delete(iter);
				FLAC__metadata_chain_delete(chain);
				return result;
			}
		}
		else
		{
			if (!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true))
			{
				result.addError("FLACFile::setVorbisComment(): Failed to append Vorbis Comment entry for " + pTagName);
				FLAC__metadata_iterator_delete(iter);
				FLAC__metadata_chain_delete(chain);
				return result;
			}
		}

		// Write the updated metadata back to the file
		if (!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		{
			result.addError("FLACFile::setVorbisComment(): Failed to write metadata to " + mFilename);
		}

		FLAC__metadata_iterator_delete(iter);
		FLAC__metadata_chain_delete(chain);
		return result;
	}

	AudioFileResultType FLACFile::getVorbisComment(const string& pTagName, string& pValue) const
	{
		AudioFileResultType result;
		pValue.clear();

		FLAC__Metadata_Chain* chain = FLAC__metadata_chain_new();
		if (!chain)
		{
			result.addError("FLACFile::getVorbisComment(): Failed to create metadata chain");
			return result;
		}

		if (!FLAC__metadata_chain_read(chain, mFilename.c_str()))
		{
			result.addError("FLACFile::getVorbisComment(): Failed to read metadata chain from " + mFilename);
			FLAC__metadata_chain_delete(chain);
			return result;
		}

		FLAC__Metadata_Iterator* iter = FLAC__metadata_iterator_new();
		if (!iter)
		{
			result.addError("FLACFile::getVorbisComment(): Failed to create metadata iterator");
			FLAC__metadata_chain_delete(chain);
			return result;
		}

		FLAC__metadata_iterator_init(iter, chain);

		// Find the VORBIS_COMMENT block
		FLAC__StreamMetadata* block = nullptr;
		do
		{
			block = FLAC__metadata_iterator_get_block(iter);
			if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
				break;
			block = nullptr;
		} while (FLAC__metadata_iterator_next(iter));

		if (block)
		{
			int idx = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, pTagName.c_str());
			if (idx >= 0)
			{
				const char* entryChars = reinterpret_cast<const char*>(block->data.vorbis_comment.comments[idx].entry);
				const char* eq = strchr(entryChars, '=');
				if (eq)
					pValue = string(eq + 1);
			}
			else
			{
				result.addError("FLACFile::getVorbisComment(): Tag '" + pTagName + "' not found in " + mFilename);
			}
		}
		else
		{
			result.addError("FLACFile::getVorbisComment(): No Vorbis Comment block found in " + mFilename);
		}

		FLAC__metadata_iterator_delete(iter);
		FLAC__metadata_chain_delete(chain);
		return result;
	}

	// MetadataAudioFile overrides - setters

	AudioFileResultType FLACFile::setTitle(const string& pTitle)
	{
		return setVorbisComment("TITLE", pTitle);
	}

	AudioFileResultType FLACFile::setArtist(const string& pArtist)
	{
		return setVorbisComment("ARTIST", pArtist);
	}

	AudioFileResultType FLACFile::setAlbum(const string& pAlbum)
	{
		return setVorbisComment("ALBUM", pAlbum);
	}

	AudioFileResultType FLACFile::setGenre(const string& pGenre)
	{
		return setVorbisComment("GENRE", pGenre);
	}

	AudioFileResultType FLACFile::setTrackNumber(uint32_t pTrackNumber, uint32_t pTotalTracks)
	{
		string trackStr = std::to_string(pTrackNumber);
		if (pTotalTracks > 0)
			trackStr += "/" + std::to_string(pTotalTracks);
		return setVorbisComment("TRACKNUMBER", trackStr);
	}

	AudioFileResultType FLACFile::setYear(const string& pYear)
	{
		return setVorbisComment("DATE", pYear);
	}

	AudioFileResultType FLACFile::setComment(const string& pComment)
	{
		return setVorbisComment("COMMENT", pComment);
	}

	// MetadataAudioFile overrides - getters

	AudioFileResultType FLACFile::getTitle(string& pTitle) const
	{
		return getVorbisComment("TITLE", pTitle);
	}

	AudioFileResultType FLACFile::getArtist(string& pArtist) const
	{
		return getVorbisComment("ARTIST", pArtist);
	}

	AudioFileResultType FLACFile::getAlbum(string& pAlbum) const
	{
		return getVorbisComment("ALBUM", pAlbum);
	}

	AudioFileResultType FLACFile::getGenre(string& pGenre) const
	{
		return getVorbisComment("GENRE", pGenre);
	}

	AudioFileResultType FLACFile::getTrackNumber(uint32_t& pTrackNumber, uint32_t& pTotalTracks) const
	{
		pTrackNumber = 0;
		pTotalTracks = 0;
		string value;
		AudioFileResultType result = getVorbisComment("TRACKNUMBER", value);
		if (result && !value.empty())
		{
			try
			{
				size_t slashPos = value.find('/');
				if (slashPos != string::npos)
				{
					pTrackNumber = static_cast<uint32_t>(std::stoul(value.substr(0, slashPos)));
					pTotalTracks = static_cast<uint32_t>(std::stoul(value.substr(slashPos + 1)));
				}
				else
				{
					pTrackNumber = static_cast<uint32_t>(std::stoul(value));
				}
			}
			catch (...)
			{
				result.addError("FLACFile::getTrackNumber(): Invalid track number value: " + value);
			}
		}
		return result;
	}

	AudioFileResultType FLACFile::getYear(string& pYear) const
	{
		return getVorbisComment("DATE", pYear);
	}

	AudioFileResultType FLACFile::getComment(string& pComment) const
	{
		return getVorbisComment("COMMENT", pComment);
	}
}
