#ifndef SOPNET_EVALUATION_RAND_INDEX_H__
#define SOPNET_EVALUATION_RAND_INDEX_H__

#include <pipeline/all.h>

#include <imageprocessing/ImageStack.h>

class RandIndex : public pipeline::SimpleProcessNode<> {

public:

	RandIndex();

private:

	void updateOutputs();

	size_t getNumAgreeingPairs(const ImageStack& stack1, const ImageStack& stack2, size_t numLocations);

	// input image stacks
	pipeline::Input<ImageStack> _stack1;
	pipeline::Input<ImageStack> _stack2;

	// variation of information
	pipeline::Output<double>    _randIndex;
};

#endif // SOPNET_EVALUATION_RAND_INDEX_H__

