#include "NeuronsMeshExtractor.h"

NeuronsMeshExtractor::NeuronsMeshExtractor() {

	registerInput(_neurons, "neurons");
	registerOutput(_meshes, "meshes");
}

void
NeuronsMeshExtractor::updateOutputs() {

	if (!_meshes)
		_meshes = new Meshes();
	else
		_meshes->clear();

	unsigned int id = 1;
	foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

		NeuronVolumeAdaptor neuronVolume(*neuron);

		boost::shared_ptr<Mesh> mesh =
				_marchingCubes.generateSurface(
					neuronVolume,
					MarchingCubes<NeuronVolumeAdaptor>::AcceptAbove(0.5),
					10.0,
					10.0,
					10.0);

		_meshes->add(id, mesh);
		id++;
	}
}
