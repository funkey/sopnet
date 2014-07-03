#ifndef SOPNET_SKELETONS_FIND_SPHERES_H__
#define SOPNET_SKELETONS_FIND_SPHERES_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/SegmentTree.h>
#include "Spheres.h"

/**
 * Find a set of spheres that approximate the shape of a neuron.
 */
class FindSpheres : public pipeline::SimpleProcessNode<> {

public:

	FindSpheres();

private:

	void updateOutputs();

	pipeline::Input<SegmentTree> _neuron;
	pipeline::Output<Spheres>    _spheres;
};

#endif // SOPNET_SKELETONS_FIND_SPHERES_H__

