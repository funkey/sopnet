#ifndef SOPNET_EVALUATION_RAND_INDEX_H__
#define SOPNET_EVALUATION_RAND_INDEX_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include "RandIndexErrors.h"

class RandIndex : public pipeline::SimpleProcessNode<> {

public:

	RandIndex();

private:

	void updateOutputs();

	size_t getNumAgreeingPairs(const ImageStack& stack1, const ImageStack& stack2, size_t& numLocations);

	// input image stacks
	pipeline::Input<ImageStack> _stack1;
	pipeline::Input<ImageStack> _stack2;

	pipeline::Output<RandIndexErrors> _errors;

	// do not count statistics for pixels that belong to the background
	bool _ignoreBackground;
};

#endif // SOPNET_EVALUATION_RAND_INDEX_H__

