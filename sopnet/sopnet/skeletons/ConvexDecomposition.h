#ifndef SOPNET_SKELETONS_CONVEX_DECOMPOSITION_H__
#define SOPNET_SKELETONS_CONVEX_DECOMPOSITION_H__

#include <pipeline/SimpleProcessNode.h>
#include <gui/Mesh.h>
#include <gui/Meshes.h>

class ConvexDecomposition : public pipeline::SimpleProcessNode<> {

public:

	ConvexDecomposition();

private:

	void updateOutputs();

	pipeline::Input<Mesh>    _mesh;
	pipeline::Output<Meshes> _convexified;

	pipeline::Input<double>       _compacityWeight;
	pipeline::Input<double>       _volumeWeight;
	pipeline::Input<double>       _connectDistance;
	pipeline::Input<unsigned int> _minNumClusters;
	pipeline::Input<unsigned int> _maxNumHullVertices;
	pipeline::Input<double>       _maxConcavity;
	pipeline::Input<double>       _smallClusterThreshold;
	pipeline::Input<unsigned int> _numTargetTriangles;
	pipeline::Input<bool>         _addExtraDistPoints;
	pipeline::Input<bool>         _addExtraFacesPoints;
};

#endif // SOPNET_SKELETONS_CONVEX_DECOMPOSITION_H__

