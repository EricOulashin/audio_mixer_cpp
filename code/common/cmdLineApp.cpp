// This is a command-line application to mix multiple audio files into a single output audio file.
// This reads a text file containing a list of audio files to mix, one per line.  The last line is the output audio filename.

#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <memory>
#include "WAVFile.h"
#include "FLACFile.h"
#include "AudioFileTools.h"
#include "AudioFileResultType.h"
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using EOUtils::AudioFile;
using EOUtils::WAVFile;
using EOUtils::FLACFile;
using EOUtils::createAudioFileObjForNewFile;
using EOUtils::mixAudioFiles;
using EOUtils::AudioFileResultType;


// For command-line options
struct ProgOptions
{
	string audioFileListFilename;
	bool verbose = false;
};

// To hold audio file mixing options
struct AudioFileMixOptions
{
	vector<string> inputFilenames;
	string outputFilename;
};

// Shows the program help
void showProgramHelp();

// Parses command-line arguments
ProgOptions parseCommandLine(int argc, char* argv[]);

// Reads the audio file list with the given filename.
AudioFileMixOptions readAudioFileList(const string& pFilename, vector<string>& pErrorMsgs, vector<string>& pNonexistentFiles);

// Main program function
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		showProgramHelp();
		exit(0);
	}

	ProgOptions progOpts = parseCommandLine(argc, argv);
	cout << "Audio file list filename: " << progOpts.audioFileListFilename << endl;

	// Read the audio file list
	vector<string> errorMessages;
	vector<string> nonexistentFiles;
	AudioFileMixOptions mixOpts = readAudioFileList(progOpts.audioFileListFilename, errorMessages, nonexistentFiles);
	// If there are any errors or non-existing input files, then show them and exit
	if (errorMessages.size() > 0)
	{
		for (const string& errMsg : errorMessages)
			cerr << errMsg << endl;
		exit(2);
	}
	if (nonexistentFiles.size() > 0)
	{
		cerr << "The following input files do not exist:\n";
		for (const string& filename : nonexistentFiles)
			cerr << filename << "\n";
		exit(3);
	}
	if (mixOpts.outputFilename.empty())
	{
		cerr << "Error: No output filename is specified in audio file list\n";
		exit(4);
	}


	// Mix the audio files
	if (progOpts.verbose)
	{
		cout << "Output file: " << mixOpts.outputFilename << "\n\n";
		cout << "Input files:\n";
		for (const string& filename : mixOpts.inputFilenames)
			cout << filename << "\n";
	}
	cout << "Mixing to " << mixOpts.outputFilename << "...\n";
	string resultMsg;
	try
	{
		shared_ptr<AudioFile> outputFile = createAudioFileObjForNewFile(mixOpts.outputFilename.c_str());
		if (outputFile != nullptr)
		{
			AudioFileResultType mixResult = mixAudioFiles(mixOpts.inputFilenames, outputFile);
			if (!mixResult)
				resultMsg = mixResult.getError();
		}
		else
			cerr << "Unable to determine the file format of the specified output file: " << mixOpts.outputFilename << "\n";
	}
	catch (const std::logic_error& e)
	{
		resultMsg = e.what();
	}
	if (resultMsg.empty())
		cout << "Mixing completed successfully.  Output file: " << mixOpts.outputFilename << endl;
	else
	{
		cerr << "Error mixing audio files: " << resultMsg << endl;
		exit(5);
	}

	return 0;
}


/////////////////// Function implementations ///////////////////
 
void showProgramHelp()
{
	cout << "AudioMixerCPP: Mix multiple audio files into a single output audio file.\n";
	cout << "Usage: AudioMixerCPP <audio_file_list_filename> [-v|--verbose]\n";
	cout << "  <audio_file_list_filename>: A text file containing a list of audio files to mix, one per line.\n";
	cout << "                              The last line is the output audio filename.\n";
	cout << "  -v, --verbose: Optional flag to enable verbose output.\n";
}

ProgOptions parseCommandLine(int argc, char* argv[])
{
	ProgOptions options;
	if (argc < 2)
		return options;

	for (int i = 0; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			showProgramHelp();
			exit(0);
		}
		else if (arg == "-v" || arg == "-V" || arg == "--verbose")
			options.verbose = true;
		else
		{
			if (std::filesystem::exists(arg))
				options.audioFileListFilename = arg;
			else
			{
				cerr << "Error: File does not exist: " << arg << endl;
				exit(1);
			}
		}
	}

	return options;
}

AudioFileMixOptions readAudioFileList(const string& pFilename, vector<string>& pErrorMsgs, vector<string>& pNonexistentFiles)
{
	pErrorMsgs.clear();
	pNonexistentFiles.clear();

	AudioFileMixOptions mixOptions;

	ifstream infile(pFilename.c_str(), std::ios::in);
	if (!infile.is_open())
	{
		pErrorMsgs.push_back("Error: Unable to open audio file list: " + pFilename);
		return mixOptions;
	}

	// Read the file into an array of strings
	vector<string> fileLines;
	string line;
	while (std::getline(infile, line))
	{
		if (line.empty())
			continue;
		if (line[0] == '#' || line[0] == ';')  // Skip comment lines
			continue;
		fileLines.push_back(line);
	}
	infile.close();

	// The last line is the output filename; check the lines above
	// to make sure the files exist
	const size_t numLines = fileLines.size();
	if (numLines < 1)
		return mixOptions;
	const size_t lastIdx = numLines - 1;
	for (size_t i = 0; i < numLines; ++i)
	{
		if (i < lastIdx)
		{
			if (std::filesystem::exists(fileLines[i]))
				mixOptions.inputFilenames.push_back(fileLines[i]);
			else
				pNonexistentFiles.push_back(fileLines[i]);
		}
		else
			mixOptions.outputFilename = fileLines[i];
	}

	return mixOptions;
}
