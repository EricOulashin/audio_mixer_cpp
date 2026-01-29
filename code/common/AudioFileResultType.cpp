#include "AudioFileResultType.h"

using std::string;
using std::list;
using std::ostream;

namespace EOUtils
{
	AudioFileResultType::AudioFileResultType(const std::string& pError)
	{
		if (!pError.empty())
			mErrors.push_back(pError);
	}

	AudioFileResultType::~AudioFileResultType()
	{
	}

	void AudioFileResultType::addError(const string& pError)
	{
		mErrors.push_back(pError);
	}

	const AudioFileResultType::errorCollectionType& AudioFileResultType::getErrors() const
	{
		return mErrors;
	}

	AudioFileResultType::errorCollectionType::const_iterator AudioFileResultType::getErrorsBegin() const
	{
		return mErrors.begin();
	}

	AudioFileResultType::errorCollectionType::const_iterator AudioFileResultType::getErrorsEnd() const
	{
		return mErrors.end();
	}

	size_t AudioFileResultType::numErrors() const
	{
		return mErrors.size();
	}

	string AudioFileResultType::getError() const
	{
		string errorStr;
		if (mErrors.size() > 0)
			errorStr = *mErrors.begin();
		return errorStr;
	}

	AudioFileResultType::operator bool() const
	{
		bool retVal = true;
		if (!mErrors.empty())
		{
			for (AudioFileResultType::errorCollectionType::const_iterator iter = mErrors.begin(); (iter != mErrors.end()) && retVal; ++iter)
				retVal = iter->empty();
		}
		return retVal;
	}

	void AudioFileResultType::outputErrors(ostream& pOutStream) const
	{
		for (errorCollectionType::const_iterator iter = mErrors.begin(); iter != mErrors.end(); ++iter)
			pOutStream << *iter << std::endl;
	}
}