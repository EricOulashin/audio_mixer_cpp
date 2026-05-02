# Audio Mixer (C++)
One of my interests and hobbies is music.  This project is able to mix multiple audio files into a
single output file, so that the audio from each source will be heard simultaneously.  Source files
may be WAV, FLAC, AIFF (`.aiff`, `.aif`, `.aifc`), Ogg Vorbis (`.ogg`, `.oga`), or MP3—subject to
successful decode support in your toolchain.

The code uses modern C++ features, such as std::thread, a lambda function, std::shared_ptr, and a
modern way of iterating through a collection.

I wrote the WAVFile class from scratch, and I used AI (Cursor) to create the classes for FLAC, MP3, Ogg,
and aiff files.

**MP3 (MPEG layer III), Ogg Vorbis (in Ogg container),** and **AIFF** PCM are implemented in `code/common`
via **libsndfile**: `MP3File`, `OggFile`, and `AiffFile` derive from `MetadataAudioFile`, through a shared
`sndfile`-backed base class (`SndFileBackedMetadataAudioFile`). Encoding quality for MP3 and Ogg writes
can be configured on those types (constant / average bitrate and VBR for MP3; Vorbis-quality or approximate
nominal bitrate for Ogg). AIFF reads and writes uncompressed PCM comparable to WAV.

Derived types (`WAVFile`, `FLACFile`, `MP3File`, …) conform to `AudioFile` / `MetadataAudioFile` so the mixer
logic can treat different formats uniformly.  For mixing audio files, the source audio files will
need to match on sample rate and channel count; the mixer also expects compatible nominal PCM layout across sources
(where reported), so keep inputs consistent similar to stacking raw WAV clips.

The following libraries are required to build this project:

**FLAC**

<ul>
<li>On Linux, the library is typically `libflac-dev` via your package manager.  There is also `libflac++-dev` for C++; this codebase does not use the C++ wrapper.
<li>For Windows, you can download the FLAC project <a href='https://github.com/xiph/flac' target='_blank'>from GitHub</a>; the header files are included, though the library files require building. You can build them with Visual Studio and CMake.
</ul>

If you want to build the Windows GUI application, set the environment variable `FLAC_LIB_DIR` to the directory where the FLAC project (as cloned from GitHub) lives on your machine.

**libsndfile** (for MP3 / Ogg / AIFF support in `code/common`)

<ul>
<li>On Debian-style Linux distributions, installing `libsndfile1-dev` (name may vary) provides headers and linkage for `-lsndfile`. The project's `code/Makefile` already links FLAC and sndfile together.
<li>MP3 *writing* normally requires libsndfile to have been built with **LAME** support; distribution packages often include this—if encoder open fails at runtime for `.mp3` output, rebuild or reinstall a full-featured libsndfile.
<li>The checked-in Visual Studio solution under `AudioMixerCPP_Win_VisualStudio` currently wires **FLAC** only; extending it to compile the sndfile-backed sources would mean adding libsndfile (and LAME-as-used-by-sndfile) to the linker settings similarly to FLAC.
</ul>

### MP3, Ogg Vorbis, and AIFF (overview)

<ul>
<li><b>MP3</b> — <code>MP3File</code> / <code>MP3FileInfo</code>; typical extension <code>.mp3</code>. Reading and writing go through libsndfile
(LAME-backed encoding when enabled in your sndfile build). Write-time bitrate control uses <code>Mp3BitrateMode</code>
<strong>Constant</strong> (CBR), <strong>Average</strong> (ABR), or <strong>Variable</strong> (VBR). Use the
<code>MP3File(path, Mp3BitrateMode, nominalKbps)</code> constructor for CBR/ABR, or
<code>MP3File(path, double)</code> for VBR (libsndfile scale: 0 = best quality, 1 = lowest). Equivalent setters
clear or override encoder defaults unless you call <code>clearMp3WriteEncodingOverrides()</code>.
<li><b>Ogg Vorbis</b> — <code>OggFile</code> / <code>OggFileInfo</code>; extensions <code>.ogg</code>, <code>.oga</code>. libsndfile only exposes quality-style
Vorbis encoding via <code>SFC_SET_COMPRESSION_LEVEL</code>. This project wraps that as <code>OggWriteQuality</code>
(1 = best perceived quality down to 0) or as <code>OggWriteApproxBitrateKbps</code> ("nominally about
<code>N</code> kb/s") using a heuristic; treat the latter as approximate, not guaranteed CBR. Setters mirror these options;
<code>clearOggWriteEncodingOverrides()</code> restores libsndfile's internal defaults on the next encode open.
<li><b>AIFF</b> — <code>AiffFile</code> / <code>AiffFileInfo</code>; PCM in classic AIFF variants with extensions such as <code>.aiff</code>, <code>.aif</code>,
and <code>.aifc</code>. No extra bitrate knobs compared to WAV—this is uncompressed PCM surfaced through libsndfile.
</ul>

There are <a href='https://www.doxygen.nl' target='_blank'>Doxygen</a> comments in the source in order to provide
documentation; there is also a Doxyfile available for building Doxygen-based documentation.
<a href='https://ericoulashin.github.io/audio_mixer_cpp/html/index.html' target='_blank'>HTML documentation</a>
for the C++ code has been added in the <a href='docs/html' target='_blank'>docs/html directory</a>.  If you
want to generate documentation, you can run `doxygen` from the directory that contains the Doxyfile (project root), and it will output
documentation into the docs directory.

In addition to the audio file classes in `code/common`, there are applications for mixing (a GUI for
Windows and a command-line tool for Linux). The Linux command-line mixer uses extensions and optional
sniffing to recognise WAV/FLAC/AIFF/Ogg/MP3 and can write `.wav`, `.flac`, `.mp3`, `.ogg`/`.oga`, or `.aiff`/
`.aif`/`.aifc` outputs based on the output filename extension.

The Windows GUI app builds in Visual Studio 2017 (I used the free Community edition).  For Visual Studio, note that
you will need to have the C++ toolset installed, along with MFC.  As for the command-line application, I've verified
it builds with g++ in Linux (I've tested it on Kubuntu 25.10).

Visual Studio can be downloaded
<a href='https://visualstudio.microsoft.com/vs/older-downloads/' target='_blank'>here</a>.
As of this writing (January 2, 2026), that page has versions of Visual Studio from 2015 to 2022.

## Code notes
In the 'code' directory, there are the following items:
<ul>
<li><b>AudioMixerCPP_Win_VisualStudio:</b> Windows-specific source & project files (for use with Visual Studio)
<li><b>common:</b> Common source files (contains the core audio & mixer code)
<li><b>AudioMixerCPP.sln:</b> The solution file for use with Visual Studio
<li><b>Makefile:</b> A makefile for building a command-line executable for Linux
</ul>

# Included Applications
The source code includes 2 applications:
<ul>
<li>A GUI application (written in MFC, for Windows) centred on WAV file selection (see filters in the VS project).
<li>A command-line application (<code>cmdLineApp.cpp</code>) aimed at Linux: it mixes the formats recognised in
<code>AudioFileTools</code>, not only WAV.
</ul>

## GUI application (Windows)
The following is a screenshot of the application:

<p align="center">
  <img src="screenshots/AudioMixerCPP.png" alt="Audio Mixer application" width="800">
</p>

To add WAV files to the list, you can drag & drop files onto the GUI, or for each line in the list,
there will be a "..." button that lets you browse and choose a WAV file to add.

## Command-line application
The command-line application source is cmdLineApp.cpp, and there is a makefile to help build everything
in Linux (g++ is used as the compiler).  The command-line application takes a command-line option which
specifies a filename containing a list of audio filenames to mix (and the last line is the output
filename to use). The command-line also accepts another parameter, -v, to enable verbose output.

# Background
A WAV audio file consists of a header at the beginning of the file, which contains strings to identify
the file type ("RIFF" and "WAVE"), as well as information about the audio contained in the file
(number of channels, sample rate, number of bits per channel, size of the data, etc.). Following the
header is all of the audio data. Digital audio data is numeric: each sample is an integer that
represents the level of the audio signal at that point in time.

The general idea behind digital audio mixing is fairly simple: Until there are no more audio samples,
the next audio sample from each audio file is read, then they're added together and saved to the output
file.  A bit more needs to be done, though, to deal with digital audio clipping.  Digital audio
clipping is caused by numeric range limitations of the values due to the sample size (i.e., 8 or 16
bits): When audio sample values are added together (or if the volume is increased), it's possible for
the resulting values to go beyond the numeric range of the values. When that happens, the result is
(often loud) pops and clicks in the audio, which is undesirable. So, in order to mix WAV files
together, mixAudioFiles() (in AudioFileTools.h and .cpp) will first analyze each audio file to
determine the highest audio sample, then reduce the volume of all the samples while mixing them to
avoid digital audio clipping.

Wikipedia has <a href='https://en.wikipedia.org/wiki/Clipping_(audio)' target='_blank'>an article on
audio clipping</a> that describes it in more detail.

## Using the code
The AudioFile class is a parent class for working with audio files, with some pure virtual methods to
be implemented in derived classes, such as the included WAVFile class.  The WAVFile class can open,
read, and write WAV audio.  Most of the class methods return an AudioFileResultType, which can be used
as if it was a bool (in an 'if' statement, for instance), and if it's false, it contains error messages
in the form of std::string.

The following are some of the more important methods in the WAVFile class:

| Method                                                                                  | Description                                                                                                                                                                                                                |
| --------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| WAVFile(const std::string& pFilename)                                                   | Constructor that takes a filename                                                                                                                                                                                          |
| WAVFile(const std::string& pFilename, const WAVFileInfo& pWAVFileInfo)                  | Constructor that takes a filename and a WAVFileInfo objecct specifying the desired properties of the WAV file (sample rate, bit rate, number of channels, etc.)                                                            |
| WAVFile(const std::string& pFilename, AudioFileModes pFileMode)                         | Constructor that takes a filename and a file mode (read, write, read/write)                                                                                                                                                |
| template <class SampleType> AudioFileResultType getNextSample(SampleType& pAudioSample) | A templatized method that reads the next sample from the audio file (if it's open in read or read/write mode).  For the template, the proper data type must be used (for instance, for 16-bit audio, you can use uint16_t. |
| template <class SampleType> AudioFileResultType writeSample(SampleType pAudioSample)    | A templated method that writes an audio sample to the WAV file (if it's open in write or read/write mode).  For the template, the proper data type must be used (for instance, for 16-bit audio, you can use uint16_t.     |


These are some important methods in the AudioFile class, which are pure virtual:

| Method                                                                 | Description                                                                                                                                                                                                                                                                                                                                                                     |
| ---------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| virtual AudioFileResultType getNextSample_int64(int64_t& pAudioSample) | Reads the next audio sample from the audio file (if in read or read/write mode).  The audio sample is cast to an int64 so that it can be used in generic algorithms - Whether the audio data is 8-bit, 16-bit, or another bitness, the audio sample is cast to 64-bit.                                                                                                          |
| virtual AudioFileResultType writeSample_int64(int64_t pAudioSample)    | Writes an audio sample to the audio file (if in write or read/write mode).  The sample parameter is an int64_t but will be cast down to the appropriate type when writing the sampel to the audio file.  This is to make generic audio algorithms simpler - Whether the audio data is 8-bit, 16-bit, or another bitness, the audio sample is cast down to the appropriate type. |

AudioFileTools.h and AudioFileTools.cpp defines the following function (among others):

| Method                                                                                             | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| -------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Various `mixAudioFiles` overloads                                              | Combine many inputs into one `AudioFile`-backed output. Decoding uses WAV/FLAC/sndfile classes as appropriate; helpers such as `createAudioFileObjForNewFile` pick the writer from the output extension (`.wav`, `.flac`, `.mp3`, `.ogg`/`.oga`, `.aiff`/`.aif`/`.aifc`). Inputs must agree on channel count and sample rate. Mixing can take noticeable time—keep heavy work off the GUI thread. |


The WAVFileInfo and AudioFileInfo classes contain information about the audio files, such as
bitness, number of channels, sample rate, etc.

An example of opening a WAV file and looping through to get each audio sample (assuming the
audio file contains 16-bit audio):
```C++
WAVFile audioFile("someAudioFile.wav");
AudioFileResultType result = audioFile.open(AUDIO_FILE_READ);
if (result)
{
    size_t numSamples = audioFile.numSamples();
    for (size_t i = 0; (i < numSamples) && result; ++i)
    {
        int16_t audioSample = 0;
        result = audioFile.getNextSample(audioSample);
    }
}
else
{
    cerr << "Error(s) getting audio samples:" << endl;
    result.outputErrors(cerr);
}
audioFile.close();
```

## Points of Interest
One interesting thing to note is that data in a WAV audio file is always little-endian, per
the specification. On big-endian systems, the byte order must be reversed before manipulating
the audio data, and the byte order for a sample must be reversed before saving it to a WAV
file. My WAVFile class handles this automatically; for example, if the system is big-endian,
then when retrieving audio samples from a WAV file or adding a 16-bit sample to a WAV file,
the byte order will be automatically reversed so that the data is in the proper order.

In creating the WAVFile class, it was necessary to look up the WAV file format specification.
I found many web pages describing the WAV file format. There were 4 URLs I had referenced
when I originally wrote this, but only one still exists today:
<ul>
<li><a href='http://www.ringthis.com/dev/wave_format.htm' target='_blank'>http://www.ringthis.com/dev/wave_format.htm</a>
</ul>

I also have <a href='https://github.com/EricOulashin/audio_mixer_c_sharp' target='_blank'>a C# version of this</a> which I originally wrote in April 2009.

I originally posted this project <a href='https://codeproject.com/articles/Cplusplus-audio-mixing-and-WAV-file-AudioFile-clas' target='_blank'>on CodeProject</a>.
