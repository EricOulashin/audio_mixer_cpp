#include "FLACFileInfo.h"
#include "utilFunctions.h"

#include <cstdio>
#include <cstring>
#include <fstream>

extern "C" {
#include <FLAC/stream_decoder.h>
}

using std::fstream;
using std::ifstream;
using std::string;

namespace EOUtils
{
	namespace
	{
		struct FLACReadClientData
		{
			std::istream* stream;
			FLAC__StreamMetadata_StreamInfo* streamInfo;
			AudioFileResultType* result;
		};

		FLAC__StreamDecoderReadStatus flacReadCallback(const FLAC__StreamDecoder* /*decoder*/, FLAC__byte buffer[], size_t* bytes, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			std::istream* stream = data->stream;
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

		FLAC__StreamDecoderSeekStatus flacSeekCallback(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64 absolute_byte_offset, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			std::istream* stream = data->stream;
			if (stream && stream->seekg(static_cast<std::streamoff>(absolute_byte_offset), std::ios_base::beg))
				return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
			return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
		}

		FLAC__StreamDecoderTellStatus flacTellCallback(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* absolute_byte_offset, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			std::istream* stream = data->stream;
			if (stream)
			{
				std::streampos pos = stream->tellg();
				if (pos >= 0)
				{
					*absolute_byte_offset = static_cast<FLAC__uint64>(pos);
					return FLAC__STREAM_DECODER_TELL_STATUS_OK;
				}
			}
			return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
		}

		FLAC__StreamDecoderLengthStatus flacLengthCallback(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* stream_length, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			std::istream* stream = data->stream;
			if (stream)
			{
				std::streampos curr = stream->tellg();
				stream->seekg(0, std::ios_base::end);
				std::streampos end = stream->tellg();
				stream->seekg(curr, std::ios_base::beg);
				if (end >= 0 && curr >= 0)
				{
					*stream_length = static_cast<FLAC__uint64>(end);
					return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
				}
			}
			return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
		}

		FLAC__bool flacEofCallback(const FLAC__StreamDecoder* /*decoder*/, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			return data->stream ? data->stream->eof() : true;
		}

		FLAC__StreamDecoderWriteStatus flacWriteCallback(const FLAC__StreamDecoder* /*decoder*/, const FLAC__Frame* /*frame*/, const FLAC__int32* const* /*buffer*/, void* /*client_data*/)
		{
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}

		void flacMetadataCallback(const FLAC__StreamDecoder* /*decoder*/, const FLAC__StreamMetadata* metadata, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO && data->streamInfo)
			{
				*data->streamInfo = metadata->data.stream_info;
			}
		}

		void flacErrorCallback(const FLAC__StreamDecoder* /*decoder*/, FLAC__StreamDecoderErrorStatus status, void* client_data)
		{
			FLACReadClientData* data = static_cast<FLACReadClientData*>(client_data);
			if (data->result)
			{
				const char* msg = FLAC__StreamDecoderErrorStatusString[status];
				data->result->addError(std::string("FLAC decoder error: ") + (msg ? msg : "unknown"));
			}
		}
	}

	FLACFileInfo::FLACFileInfo(int16_t pNumChannels, int32_t pSampleRateHz, int32_t pBytesPerSecond, int16_t pByteRate, int16_t pBitsPerSample)
		: AudioFileInfo(pNumChannels, pSampleRateHz, pBytesPerSecond, pByteRate, pBitsPerSample)
	{
	}

	FLACFileInfo::FLACFileInfo(const AudioFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo)
	{
	}

	FLACFileInfo::FLACFileInfo(const FLACFileInfo& pAudioFileInfo)
		: AudioFileInfo(pAudioFileInfo),
		  mCompressionLevel(pAudioFileInfo.mCompressionLevel)
	{
	}

	AudioFileResultType FLACFileInfo::read(std::fstream& pInFStream)
	{
		AudioFileResultType result;

		if (!pInFStream.is_open())
		{
			result.addError("FLACFileInfo::read(): The file is not open");
			return result;
		}

		pInFStream.seekg(0, std::ios_base::beg);

		FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
		if (!decoder)
		{
			result.addError("FLACFileInfo::read(): Failed to create FLAC decoder");
			return result;
		}

		FLAC__StreamMetadata_StreamInfo streamInfo;
		std::memset(&streamInfo, 0, sizeof(streamInfo));

		FLACReadClientData clientData;
		clientData.stream = &pInFStream;
		clientData.streamInfo = &streamInfo;
		clientData.result = &result;

		FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(
			decoder,
			flacReadCallback,
			flacSeekCallback,
			flacTellCallback,
			flacLengthCallback,
			flacEofCallback,
			flacWriteCallback,
			flacMetadataCallback,
			flacErrorCallback,
			&clientData
		);

		if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
		{
			result.addError(std::string("FLACFileInfo::read(): Failed to init decoder: ") +
				(FLAC__StreamDecoderInitStatusString[initStatus] ? FLAC__StreamDecoderInitStatusString[initStatus] : "unknown"));
			FLAC__stream_decoder_delete(decoder);
			return result;
		}

		FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_metadata(decoder);
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);

		if (!ok && result.numErrors() == 0)
			result.addError("FLACFileInfo::read(): Failed to read FLAC metadata");

		if (result)
		{
			mNumChannels = static_cast<int16_t>(streamInfo.channels);
			mSampleRateHz = static_cast<int32_t>(streamInfo.sample_rate);
			mBitsPerSample = static_cast<int16_t>(streamInfo.bits_per_sample);
			mByteRate = static_cast<int16_t>(mNumChannels * (mBitsPerSample / BITS_PER_BYTE));
			mBytesPerSecond = mSampleRateHz * static_cast<int32_t>(mByteRate);
			mFileSize = static_cast<int32_t>(streamInfo.total_samples * streamInfo.channels * (streamInfo.bits_per_sample / 8));
		}

		return result;
	}

	AudioFileResultType FLACFileInfo::write(std::fstream& /*pOutFStream*/)
	{
		AudioFileResultType result;
		result.addError("FLACFileInfo::write(): FLAC metadata is written by the encoder, not directly");
		return result;
	}

	AudioFileResultType FLACFileInfo::read(const char* pFilename)
	{
		AudioFileResultType result;
		fstream inFile(pFilename, std::ios_base::binary | std::ios_base::in);
		if (inFile.is_open())
		{
			result = read(inFile);
			inFile.close();
		}
		else
			result.addError(std::string("FLACFileInfo::read(): Failed to open ") + pFilename + " for reading");
		return result;
	}

	bool FLACFileInfo::isFLACFile(const char* pFilename)
	{
		if (!pFilename)
			return false;

		ifstream file(pFilename, std::ios_base::binary | std::ios_base::in);
		if (!file.is_open())
			return false;

		char signature[4];
		file.read(signature, 4);
		if (file.gcount() != 4)
			return false;

		return (signature[0] == 'f' && signature[1] == 'L' && signature[2] == 'a' && signature[3] == 'C');
	}

	uint32_t FLACFileInfo::CompressionLevel() const
	{
		return mCompressionLevel;
	}

	void FLACFileInfo::CompressionLevel(uint32_t pCompressionLevel)
	{
		// The compression level must be between 0 and 8 (inclusive), where 0 is fastest and 8 is maximum compression.
		// pCompressionLevel is unsigned, so only need to check the upper bound.
		if (pCompressionLevel <= 8)
			mCompressionLevel = pCompressionLevel;
	}
}
