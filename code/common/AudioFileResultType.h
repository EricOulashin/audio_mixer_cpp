#ifndef __AUDIO_FILE_RESULT_TYPE_H__
#define __AUDIO_FILE_RESULT_TYPE_H__


#include <string>
#include <list>
#include <iterator>
#include <ostream>

namespace EOUtils
{
	class AudioFileResultType
	{
		public:
			using errorCollectionType = std::list<std::string>;

			AudioFileResultType(const std::string& pError = "");

			~AudioFileResultType();

			void addError(const std::string& pError);

			const errorCollectionType& getErrors() const;

			errorCollectionType::const_iterator getErrorsBegin() const;

			errorCollectionType::const_iterator getErrorsEnd() const;

			size_t numErrors() const;

			std::string getError() const;

			operator bool() const;

			void outputErrors(std::ostream& pOutStream) const;

		private:
			errorCollectionType mErrors;
	};
}

#endif