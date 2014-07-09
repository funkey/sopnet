#ifndef SOPNET_SKELETONS_FIND_SPHERES_H__
#define SOPNET_SKELETONS_FIND_SPHERES_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
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

	void getBoundingBox(
			const SegmentTree& neuron,
			double& minX, double& maxX,
			double& minY, double& maxY,
			double& minZ, double& maxZ);

	pipeline::Input<SegmentTree> _neuron;
	pipeline::Input<double>      _smooth;
	pipeline::Output<Spheres>    _spheres;
	pipeline::Output<ImageStack> _maxDistances;
};

#endif // SOPNET_SKELETONS_FIND_SPHERES_H__

