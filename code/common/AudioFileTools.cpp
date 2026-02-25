
#include "AudioFileTools.h"
#include "utilFunctions.h"
#include "WAVFileInfo.h"
#include "WAVFile.h"
#include "FLACFileInfo.h"
#include "FLACFile.h"

#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <memory>
#include <filesystem>
#include <cstring>

using std::fstream;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using EOUtils::AudioFile;
using EOUtils::AudioFileResultType;
using EOUtils::AudioFileInfo;


shared_ptr<AudioFile> EOUtils::createAudioFileObjForExistingFile(const char* pFilename)
{
	if (!std::filesystem::exists(pFilename))
		return nullptr;

	// Find the filename extension from pFilename and decide the file format
	// based on that
	shared_ptr<AudioFile> audioFile;
	if (WAVFileInfo::isWAVFile(pFilename))
		audioFile = make_shared<WAVFile>(pFilename);
	else if (FLACFileInfo::isFLACFile(pFilename))
		audioFile = make_shared<FLACFile>(pFilename);
	return audioFile;
}

shared_ptr<AudioFile> EOUtils::createAudioFileObjForNewFile(const char* pFilename)
{
	// Find the filename extension from pOutputFilename and decide the file format
	// based on that.  If unknown, output an error message.
	const string filenameExt = getFileExtensionUpper(pFilename);
	if (filenameExt.empty())
		return nullptr;

	shared_ptr<AudioFile> outputFile;
	if (filenameExt == "WAV")
		outputFile = make_shared<WAVFile>(pFilename);
	else if (filenameExt == "FLAC")
		outputFile = make_shared<FLACFile>(pFilename);

	return outputFile;
}

string EOUtils::getFileExtensionUpper(const char* pFilename)
{
	if (pFilename == nullptr || pFilename[0] == '\0')
		return "";

	// Look for a dot in piIlename
	char *dot = nullptr;
	size_t len = strlen(pFilename);
	for (size_t i = len - 1; i > 0; --i)
	{
		if (pFilename[i] == '.')
		{
			dot = (char*)(pFilename + i);
			break;
		}
	}
	if (dot == nullptr)
		return "";

	string filenameExt = string(dot + 1);
	// Convert the string to uppercase in-place
	std::transform(filenameExt.begin(), filenameExt.end(), filenameExt.begin(),  [](unsigned char c){ return std::toupper(c); });
	return filenameExt;
}

AudioFileResultType EOUtils::getAudioFileInfo(const char* pFilename, AudioFileInfo& pAudioFileInfo)
{
	AudioFileResultType result;

	unique_ptr<AudioFileInfo> fileInfo;
	if (WAVFileInfo::isWAVFile(pFilename))
		fileInfo = make_unique<WAVFileInfo>();
	else if (FLACFileInfo::isFLACFile(pFilename))
		fileInfo = make_unique<FLACFileInfo>();
	else
	{
		result.addError("Unrecognized audio file format for " + string(pFilename));
		return result;
	}

	if (fileInfo != nullptr)
	{
		fstream inFile;
		inFile.open(pFilename, std::ios_base::binary | std::ios_base::in);
		if (inFile.is_open())
		{
			result = fileInfo->read(inFile);
			inFile.close();
			if (result && result.numErrors() == 0)
				pAudioFileInfo.copyAudioFileInfo(*fileInfo);
		}
		else
			result.addError("Failed to open " + string(pFilename) + " for reading");
	}

	return result;
}

AudioFileResultType EOUtils::mixAudioFiles(const vector<string>& pFilenames, const string& pOutputFilename)
{
	if (pOutputFilename.empty())
	{
		AudioFileResultType result;
		result.addError("No output filename provided");
		return result;
	}
	// Try to create an AudioFile object for the output file, detecting what format it should be
	shared_ptr<AudioFile> finalOutputFile = createAudioFileObjForNewFile(pOutputFilename.c_str());
	if (finalOutputFile == nullptr)
	{
		AudioFileResultType result;
		result.addError("Unable to determine file type for " + pOutputFilename);
		return result;
	}

	return mixAudioFiles(pFilenames, *finalOutputFile);
}

AudioFileResultType EOUtils::mixAudioFiles(const std::vector<std::string>& pFilenames, std::shared_ptr<AudioFile>& pOutputAudioFile)
{
	return mixAudioFiles(pFilenames, *pOutputAudioFile);
}

AudioFileResultType EOUtils::mixAudioFiles(const std::vector<std::string>& pFilenames, EOUtils::AudioFile& pOutputAudioFile)
{
	AudioFileResultType result;

	if (pFilenames.size() == 0)
	{
		result.addError("An empty filename list was provided");
		return result;
	}

	// Ensure all the audio files have the same sample rate and number of channels.  Also, get the highest number
	// of bits per sample for the audio files.
	AudioFileInfo fileInfo;
	int16_t highestNumBitsPerSample = 0;
	result = getAudioFileInfo(pFilenames[0].c_str(), fileInfo);
	if (result)
	{
		int16_t lastNumchannels = 0;
		int32_t lastSampleRateHz = 0;
		highestNumBitsPerSample = fileInfo.BitsPerSample();
		for (size_t i = 1; (i < pFilenames.size()) && result; ++i)
		{
			lastNumchannels = fileInfo.NumChannels();
			lastSampleRateHz = fileInfo.SampleRateHz();
			result = getAudioFileInfo(pFilenames[i].c_str(), fileInfo);
			if (result)
			{
				if (fileInfo.NumChannels() != lastNumchannels)
					result.addError("Not all audio files given have the same number of channels");
				if (fileInfo.SampleRateHz() != lastSampleRateHz)
					result.addError("Not all audio files given have the same sample rate");
				if (!result)
					break;
				if (fileInfo.BitsPerSample() > highestNumBitsPerSample)
					highestNumBitsPerSample = fileInfo.BitsPerSample();
			}
		}
	}
	if (!result)
		return result;


	// Calculate the multiplier to use to scale down the sound files
	double multiplier = 0.0;
	int64_t highestSampleValueInSourceFiles = 0;
	result = getHighestSampleValue_64bit(pFilenames, highestSampleValueInSourceFiles);
	if (result)
	{
		int64_t difference = 0;
		// This will be different depending on whether there are 8 bits/sample or
		// 16 bits/sample for the sound files.
		if (highestNumBitsPerSample == 8)
			difference = (int64_t)(highestSampleValueInSourceFiles - ((int64_t)maxValue<uint8_t>() / (int64_t)pFilenames.size()));
		else if (highestNumBitsPerSample == 16)
			difference = (int64_t)(highestSampleValueInSourceFiles - ((int64_t)maxValue<int16_t>() / (int64_t)pFilenames.size()));
		else if (highestNumBitsPerSample == 32)
			difference = (int64_t)(highestSampleValueInSourceFiles - ((int64_t)maxValue<int32_t>() / (int64_t)pFilenames.size()));
		multiplier = 1.0 - ((double)difference / (double)highestSampleValueInSourceFiles);
	}
	// Proceed even if getHighestSampleValue failed (FLAC can return 'end of stream'); use multiplier 1.0
	result = AudioFileResultType();  // Reset so mixing loop runs
	if (multiplier == 0.0)
		multiplier = 1.0;  // No scaling when we couldn't determine peak

	if (multiplier < 0.0)
		multiplier = -multiplier;

	// Create a collection of AudioFile objects for the audio files, and get the highest number of samples of all the files.
	// This section of code also opens the source files.
	const std::string sampleCountMetadataName = "sampleCount";
	size_t highestNumSamples = 0;
	vector<shared_ptr<AudioFile>> srcFiles;
	for (const string& filename : pFilenames)
	{
		shared_ptr<AudioFile> audioFile = createAudioFileObjForExistingFile(filename.c_str());
		if (audioFile == nullptr)
			result.addError("Unable to determine file type for " + filename);
		else
		{
			result = audioFile->open(AUDIO_FILE_READ);
			if (result)
			{
				srcFiles.push_back(audioFile);
				size_t numSamples = audioFile->numSamples();
				audioFile->setMetadataFromVal(sampleCountMetadataName, numSamples);
				if (numSamples > highestNumSamples)
					highestNumSamples = audioFile->numSamples();
			}
			else
				break;
		}
	}
	if (!result)
		return result;

	// Open the source files
	for (shared_ptr<AudioFile>& srcFile : srcFiles)
		result = srcFile->open(AUDIO_FILE_READ);
	if (!result)
		return result;

	// Do the mixing. First mix to a lossless format, then increase the
	// volume back to where it should be in the desired output format.
	// Open a temporary file for the initial mixing.
	const string finalOutputFilename = pOutputAudioFile.Filename();
	const string mixedFilename = finalOutputFilename + "-mixed_tmp.flac";
	unique_ptr<AudioFile> mixedTmpFile = make_unique<FLACFile>(mixedFilename, 8); // 8: Maximum FLAC compression
	mixedTmpFile->setAudioFileInfo(srcFiles[0]->getAudioFileInfo());
	mixedTmpFile->setMetadataFromVal("totalSamples", highestNumSamples);
	result = mixedTmpFile->open(AUDIO_FILE_WRITE);
	if (!result)
	{
		// Close all the source files before returning
		for (shared_ptr<AudioFile>& srcFile : srcFiles)
			srcFile->close();
		return result;
	}

	// Mix the audio files into the destination file.  And keep track of the highest sample value
	// to use when increasing the destination volume later.
	// The basic algorithm for doing the mixing is as follows:
	// while there is at least 1 sample remaining in any of the source files
	//    sample = 0
	//    for each source file
	//       if the source file has any samples remaining
	//          sample = sample + (next available sample from the source file * multiplier)
	//    sample = sample / # of source files
	//    write the sample to the output file
	int64_t sample = 0;
	int64_t srcSample = 0;
	int64_t finalMixFileHighestSample = 0;
	for (size_t sampleIdx = 0; (sampleIdx < highestNumSamples) && result; ++sampleIdx)
	{
		sample = 0;
		for (size_t fileIdx = 0; (fileIdx < srcFiles.size()) && result; ++fileIdx)
		{
			if (sampleIdx < srcFiles[fileIdx]->getMetadataAs<size_t>(sampleCountMetadataName))
			{
				result = srcFiles[fileIdx]->getNextSample_int64(srcSample);
				if (result)
					sample += (int32_t)(srcSample * multiplier);
			}
		}
		if (result)
		{
			sample /= (int32_t)(srcFiles.size());
			AudioFileResultType writeRes = mixedTmpFile->writeSample_int64(sample);
			if (!writeRes)
				result = writeRes;
			if (sample > finalMixFileHighestSample)
				finalMixFileHighestSample = sample;
		}
		else
		{
			// Close all the source files before returning
			for (shared_ptr<AudioFile>& srcFile : srcFiles)
				srcFile->close();
			return result;
		}
	}

	// Close all the source files
	for (shared_ptr<AudioFile>& srcFile : srcFiles)
		srcFile->close();

	// Increase the destination file volume.
	// First, calculate the volume multiplier
	multiplier = (double)mixedTmpFile->maxValueForSampleSize() / (double)finalMixFileHighestSample;
	if (multiplier < 0.0)
		multiplier = -multiplier;
	// Get the audio file information from the temporary mixed file, then close it.
	// We'll re-open it so that we can start at the beginning and increase the volume.
	AudioFileInfo audioFileInfo = mixedTmpFile->getAudioFileInfo();
	mixedTmpFile->close();
	//audioFileInfo.FileSize(0);
	bool openedOutputFileHere = false;
	if (!pOutputAudioFile.isOpen())
	{
		pOutputAudioFile.setAudioFileInfo(audioFileInfo);
		AudioFileResultType openResult = pOutputAudioFile.open(AUDIO_FILE_WRITE);
		if (openResult && pOutputAudioFile.isOpen())
			openedOutputFileHere = true;
		else
		{
			result.addError("Unable to open " + finalOutputFilename + " for writing:");
			const auto& errorMsgs = openResult.getErrors();
			for (const string& errMsg : errorMsgs)
				result.addError(errMsg);
			return result;
		}
	}
	mixedTmpFile = make_unique<FLACFile>(mixedFilename);
	result = mixedTmpFile->open(AUDIO_FILE_READ);
	if (!result || !mixedTmpFile->isOpen())
	{
		if (openedOutputFileHere)
			pOutputAudioFile.close();
		if (!result)
			result.addError("Unable to open " + mixedFilename + " for reading");
		return result;
	}
	// Copy samples from mixed tmp to final output with volume scaling.
	// Use highestNumSamples (we know exactly how many we wrote) rather than file's numSamples(),
	// which can be 0 if FLAC lacks total_samples in metadata.
	for (size_t i = 0; (i < highestNumSamples) && result; ++i)
	{
		result = mixedTmpFile->getNextSample_int64(sample);
		if (result)
			result = pOutputAudioFile.writeSample_int64((int64_t)(sample * multiplier));
	}
	mixedTmpFile->close();
	if (openedOutputFileHere)
		pOutputAudioFile.close();

	// Remove the initial mixed file and keep the one with the increased volume
	if (remove(mixedFilename.c_str()) == 0)
	{
		if (rename(mixedFilename.c_str(), finalOutputFilename.c_str()) != 0)
		{
			// Not sure why this is failing sometimes, but as a kludge, check whether the file exists too
			if (!std::filesystem::exists(finalOutputFilename))
				result.addError("Unable to rename " + mixedFilename + " to " + finalOutputFilename);
		}
	}
	else
		result.addError("File access error with " + mixedFilename);

	return result;
}

AudioFileResultType EOUtils::getHighestSampleValue_64bit(const std::vector<std::string>& pFilenames, int64_t& pHighestAudioSample)
{
	pHighestAudioSample = 0;
	int64_t highestAudioSample = 0;
	AudioFileResultType result;
	for (size_t i = 0; (i < pFilenames.size()) && result; ++i)
	{
		shared_ptr<AudioFile> audioFile = createAudioFileObjForExistingFile(pFilenames[i].c_str());
		if (audioFile != nullptr)
		{
			result = audioFile->open(AUDIO_FILE_READ);
			if (result && audioFile->isOpen())
			{
				result = audioFile->getHighestSampleValue_int64(highestAudioSample);
				if (result && (highestAudioSample > pHighestAudioSample))
					pHighestAudioSample = highestAudioSample;
				audioFile->close();
			}
		}
		else
			result.addError("Could not open " + pFilenames[i] + " for reading");
	}
	return result;
}