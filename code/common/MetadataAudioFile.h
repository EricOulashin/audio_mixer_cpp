#ifndef __EO_UTILS_METADATA_AUDIO_FILE_H__
#define __EO_UTILS_METADATA_AUDIO_FILE_H__

#include <string>
#include <cstdint>

#include "AudioFile.h"

namespace EOUtils
{
	/**
	 * @brief Abstract class that extends AudioFile with pure virtual methods
	 * for adding/updating common audio metadata (tags).  Derived classes for
	 * audio formats that support metadata (e.g., FLAC, Ogg Vorbis) should
	 * inherit from this class and implement the metadata operations.
	 */
	class MetadataAudioFile : public AudioFile
	{
		public:
			explicit MetadataAudioFile(const std::string& pFilename);
			explicit MetadataAudioFile(const std::string& pFilename, AudioFileModes pFileMode);
			MetadataAudioFile(const MetadataAudioFile& pOther);
			virtual ~MetadataAudioFile();

			/**
			 * @brief Sets the title metadata tag
			 *
			 * @param[in] pTitle The title string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setTitle(const std::string& pTitle) = 0;

			/**
			 * @brief Sets the artist metadata tag
			 *
			 * @param[in] pArtist The artist string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setArtist(const std::string& pArtist) = 0;

			/**
			 * @brief Sets the album metadata tag
			 *
			 * @param[in] pAlbum The album string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setAlbum(const std::string& pAlbum) = 0;

			/**
			 * @brief Sets the genre metadata tag
			 *
			 * @param[in] pGenre The genre string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setGenre(const std::string& pGenre) = 0;

			/**
			 * @brief Sets the track number metadata tag
			 *
			 * @param[in] pTrackNumber The track number
			 * @param[in] pTotalTracks The total number of tracks (0 means not specified, stores only the track number)
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setTrackNumber(uint32_t pTrackNumber, uint32_t pTotalTracks = 0) = 0;

			/**
			 * @brief Sets the year/date metadata tag
			 *
			 * @param[in] pYear The year string (e.g., "2024")
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setYear(const std::string& pYear) = 0;

			/**
			 * @brief Sets the comment metadata tag
			 *
			 * @param[in] pComment The comment string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType setComment(const std::string& pComment) = 0;

			/**
			 * @brief Gets the title metadata tag
			 *
			 * @param[out] pTitle Will receive the title string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getTitle(std::string& pTitle) const = 0;

			/**
			 * @brief Gets the artist metadata tag
			 *
			 * @param[out] pArtist Will receive the artist string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getArtist(std::string& pArtist) const = 0;

			/**
			 * @brief Gets the album metadata tag
			 *
			 * @param[out] pAlbum Will receive the album string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getAlbum(std::string& pAlbum) const = 0;

			/**
			 * @brief Gets the genre metadata tag
			 *
			 * @param[out] pGenre Will receive the genre string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getGenre(std::string& pGenre) const = 0;

			/**
			 * @brief Gets the track number metadata tag
			 *
			 * @param[out] pTrackNumber Will receive the track number
			 * @param[out] pTotalTracks Will receive the total number of tracks (0 if not specified in the metadata)
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getTrackNumber(uint32_t& pTrackNumber, uint32_t& pTotalTracks) const = 0;

			/**
			 * @brief Gets the year/date metadata tag
			 *
			 * @param[out] pYear Will receive the year string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getYear(std::string& pYear) const = 0;

			/**
			 * @brief Gets the comment metadata tag
			 *
			 * @param[out] pComment Will receive the comment string
			 *
			 * @return An AudioFileResultType with errors on failure
			 */
			virtual AudioFileResultType getComment(std::string& pComment) const = 0;

			/** Default: codecs without lossy target-bitrate control (covers FLAC lossless baseline; subclasses may override). */
			bool BitrateIsAdjustable() const override;

	};
}

#endif
