#ifndef SOPNET_GUI_NEURONS_MESH_EXTRACTOR_H__
#define SOPNET_GUI_NEURONS_MESH_EXTRACTOR_H__

#include <pipeline/SimpleProcessNode.h>
#include <gui/Meshes.h>
#include <gui/MarchingCubes.h>
#include <sopnet/segments/SegmentTrees.h>
#include "NeuronVolumeAdaptor.h"

class NeuronsMeshExtractor : public pipeline::SimpleProcessNode<> {

public:

	NeuronsMeshExtractor();

private:

	void updateOutputs();

	pipeline::Input<SegmentTrees> _neurons;
	pipeline::Output<Meshes>      _meshes;

	MarchingCubes<NeuronVolumeAdaptor> _marchingCubes;
};

#endif // SOPNET_GUI_NEURONS_MESH_EXTRACTOR_H__

