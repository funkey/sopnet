#include <util/Logger.h>
#include <util/exceptions.h>
#include "RandIndex.h"

logger::LogChannel randindexlog("randindexlog", "[ResultEvaluator] ");

RandIndex::RandIndex() :
	_randIndex(new double(0)) {

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

	size_t numLocations = depth*width*height;

	if (numLocations == 0) {

		*_randIndex = 1.0;
		return;
	}

	size_t numAgree = getNumAgreeingPairs(*_stack1, *_stack2, numLocations);
	double numPairs = (static_cast<double>(numLocations)/2)*(static_cast<double>(numLocations) - 1);

	LOG_DEBUG(randindexlog) << "number of pairs is          " << numPairs << std::endl;;
	LOG_DEBUG(randindexlog) << "number of agreeing pairs is " << numAgree << std::endl;;

	*_randIndex = static_cast<double>(numAgree)/numPairs;

	// dump to output (useful for redirection into file)
	LOG_USER(randindexlog) << "# RAND" << std::endl;
	LOG_USER(randindexlog) << (*_randIndex) << std::endl;
}

size_t
RandIndex::getNumAgreeingPairs(const ImageStack& stack1, const ImageStack& stack2, size_t numLocations) {

	// Implementation following algorith by Bjoern Andres:
	//
	// https://github.com/bjoern-andres/partition-comparison/blob/master/include/andres/partition-comparison.hxx

	typedef float                       Label;
	typedef std::pair<Label,    Label>  LabelPair;
	typedef std::map<LabelPair, size_t> ContingencyMatrix;
	typedef std::map<Label,     size_t> SumVector;

	ContingencyMatrix c;
	SumVector         a;
	SumVector         b;

	ImageStack::const_iterator image1 = stack1.begin();
	ImageStack::const_iterator image2 = stack2.begin();

	// for each image in the stacks
	for(; image1 != stack1.end(); image1++, image2++) {

		Image::iterator i1 = (*image1)->begin();
		Image::iterator i2 = (*image2)->begin();

		for (; i1 != (*image1)->end(); i1++, i2++) {

			c[std::make_pair(*i1, *i2)]++;
			a[*i1]++;
			b[*i2]++;
		}
	}

	LabelPair labelPair;
	Label     label;
	size_t    n;

	size_t A = 0;
	size_t B = numLocations*numLocations;

	foreach (boost::tie(labelPair, n), c) {

		A += n*(n-1);
		B += n*n;
	}

	foreach (boost::tie(label, n), a)
		B -= n*n;
	foreach (boost::tie(label, n), b)
		B -= n*n;

	return (A+B)/2;
}
