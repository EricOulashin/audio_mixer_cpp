#ifndef __EO_UTILS_AIFF_FILE_H__
#define __EO_UTILS_AIFF_FILE_H__

#include <string>
#include <utility>

#include "SndFileBackedMetadataAudioFile.h"
#include "AiffFileInfo.h"

namespace EOUtils
{
	class AiffFile : public SndFileBackedMetadataAudioFile
	{
		public:
			explicit AiffFile(const std::string& pFilename);

			explicit AiffFile(const std::string& pFilename, const AiffFileInfo& pFileInfo);

			explicit AiffFile(const std::string& pFilename, AudioFileModes pFileMode);

			AiffFile(const AiffFile& pOther);

			AiffFile(AiffFile&& pOther) noexcept;

			~AiffFile() override;

			AudioFileResultType open(AudioFileModes pOpenMode) override;

			void setAudioFileInfo(const AudioFileInfo& pAudioFileInfo) override;

			/** PCM in AIFF: stream size follows sample layout, not a lossy bitrate setting. */
			bool BitrateIsAdjustable() const override;

			const AiffFileInfo& getFileInfo() const;

		protected:
			bool readFormatMatches(int pSfFullFormat) const override;

			int newFileSfFormatMask() const override;

			const char* formatLabel() const override;

		private:
			AiffFileInfo mAiffInfo;
	};
}

#endif
