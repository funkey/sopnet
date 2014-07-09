#ifndef SOPNET_SKELETONS_FIND_SUPERPIXELS_H__
#define SOPNET_SKELETONS_FIND_SUPERPIXELS_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include "Spheres.h"

class FindSuperPixels : public pipeline::SimpleProcessNode<> {

public:

	FindSuperPixels();

private:

	void updateOutputs();

	pipeline::Input<ImageStack> _boundaryMap;
	pipeline::Input<Spheres>    _seeds;

	pipeline::Output<ImageStack> _labels;
};

#endif // SOPNET_SKELETONS_FIND_SUPERPIXELS_H__

