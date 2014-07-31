#ifndef SOPNET_SKELETONS_FIND_INTERIOR_POINTS_H__
#define SOPNET_SKELETONS_FIND_INTERIOR_POINTS_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include "Spheres.h"

/**
 * Find a set of spheres that approximate the shape of a neuron.
 */
class FindInteriorPoints : public pipeline::SimpleProcessNode<> {

public:

	FindInteriorPoints();

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _membranes;
	pipeline::Input<double>      _smooth;
	pipeline::Output<Spheres>    _spheres;
	pipeline::Output<ImageStack> _boundaries;
};

#endif // SOPNET_SKELETONS_FIND_INTERIOR_POINTS_H__

