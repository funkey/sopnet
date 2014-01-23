#include <util/Logger.h>
#include <util/exceptions.h>
#include "RandIndex.h"

logger::LogChannel randindexlog("randindexlog", "[ResultEvaluator] ");

RandIndex::RandIndex() {

	registerInput(_stack1, "stack 1");
	registerInput(_stack2, "stack 2");
	registerOutput(_randIndex, "rand index");
}

void
RandIndex::updateOutputs() {

	if (_stack1->size() != _stack2->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("image stacks have different size") << STACK_TRACE);

	unsigned int depth = _stack1->size();

	if (depth == 0) {

		*_randIndex = 1.0;
		return;
	}

	unsigned int width  = (*_stack1)[0]->width();
	unsigned int height = (*_stack1)[0]->height();

	unsigned int numLocations = depth*width*height;

	LOG_DEBUG(randindexlog) << "preparing label vectors for " << numLocations << " locations" << std::endl;

	std::vector<float> partition1(numLocations);
	std::vector<float> partition2(numLocations);

	// count label occurences

	ImageStack::const_iterator s1  = _stack1->begin();
	ImageStack::const_iterator s2  = _stack2->begin();

	unsigned int n = 0;

	LOG_DEBUG(randindexlog) << "filling label vectors" << std::endl;

	for (; s1 != _stack1->end(); s1++, s2++) {

		if ((*s1)->size() != (*s2)->size())
			BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("images have different size") << STACK_TRACE);

		Image::iterator j1 = (*s1)->begin();
		Image::iterator j2 = (*s2)->begin();

		for (; j1 != (*s1)->end(); j1++, j2++) {

			partition1[n] = *j1;
			partition2[n] = *j2;
			n++;
		}
	}

	LOG_DEBUG(randindexlog) << "filled " << n << " entries in label vector" << std::endl;

	LOG_DEBUG(randindexlog) << "counting number of agreeing pairs" << std::endl;

	double numAgree = 0;
	for (unsigned int i = 0; i < n; i++)
		for (unsigned int j = i + 1; j < n; j++)
			if (
					(partition1[i] == partition1[j] && partition2[i] == partition2[j]) ||
					(partition1[i] != partition1[j] && partition2[i] != partition2[j]))
				numAgree++;

	double numPairs = ((double)n*((double)n - 1))/2;

	LOG_DEBUG(randindexlog) << "number of pairs is          " << numPairs << std::endl;;
	LOG_DEBUG(randindexlog) << "number of agreeing pairs is " << numAgree << std::endl;;

	*_randIndex = numAgree/numPairs;

	// dump to output (useful for redirection into file)
	LOG_USER(randindexlog) << "# RAND" << std::endl;
	LOG_USER(randindexlog) << (*_randIndex) << std::endl;
}
