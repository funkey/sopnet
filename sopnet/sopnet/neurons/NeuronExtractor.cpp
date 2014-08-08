#include "NeuronExtractor.h"
#include <util/Logger.h>

logger::LogChannel neuronextractorlog("neuronextractorlog", "[NeuronExtractor] ");

NeuronExtractor::NeuronExtractor() {

	registerInput(_segments, "segments");
	registerOutput(_neurons, "neurons");
}

void
NeuronExtractor::updateOutputs() {

	if (!_neurons)
		_neurons = new SegmentTrees();

	_slices.clear();
	_neuronIds.clear();
	_neurons->clear();

	LOG_ALL(neuronextractorlog) << "processing " << _segments->getEnds().size() << " ends" << std::endl;

	// collect all end slices
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		addSlice(end->getSlice()->getId());

	LOG_ALL(neuronextractorlog) << "processing " << _segments->getContinuations().size() << " continuations" << std::endl;

	// identify slices belonging to the same neuron
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		mergeSlices(
				continuation->getSourceSlice()->getId(),
				continuation->getTargetSlice()->getId());
	}

	LOG_ALL(neuronextractorlog) << "processing " << _segments->getBranches().size() << " branches" << std::endl;

	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		mergeSlices(
				branch->getSourceSlice()->getId(),
				branch->getTargetSlice1()->getId());

		mergeSlices(
				branch->getSourceSlice()->getId(),
				branch->getTargetSlice2()->getId());
	}

	LOG_ALL(neuronextractorlog) << "found " << _slices.size() << " slices" << std::endl;

	// assign a neuron id to each slice

	unsigned int sliceId;
	std::set<unsigned int> sameNeuronSlices;

	unsigned int neuronId = 0;

	foreach (boost::tie(sliceId, sameNeuronSlices), _slices) {

		bool alreadyAssigned = false;

		foreach (unsigned int id, sameNeuronSlices) {

			if (!_neuronIds.count(id)) {

				_neuronIds[id] = neuronId;

			} else {

				alreadyAssigned = true;
				break;
			}
		}

		if (!alreadyAssigned)
			neuronId++;
	}

	// sort segments according to the neuron their slices belong to

	// prepare neurons
	std::vector<boost::shared_ptr<SegmentTree> > neurons(neuronId);
	for (unsigned int i = 0; i < neuronId; i++) {

		neurons[i] = boost::make_shared<SegmentTree>();
		neurons[i]->setResolution(
				_segments->getResolutionX(),
				_segments->getResolutionY(),
				_segments->getResolutionZ());
	}

	// collect all end segments
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		unsigned int id = _neuronIds[end->getSlice()->getId()];

		neurons[id]->add(end);
	}

	// collect all continuation segments
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		unsigned int id = _neuronIds[continuation->getSourceSlice()->getId()];

		neurons[id]->add(continuation);
	}

	// collect all branch segments
	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		unsigned int id = _neuronIds[branch->getSourceSlice()->getId()];

		neurons[id]->add(branch);
	}

	// finally, put found neurons in output data structure

	foreach (boost::shared_ptr<SegmentTree> neuron, neurons)
		_neurons->add(neuron);

	LOG_ALL(neuronextractorlog) << "found " << _neurons->size() << " neurons" << std::endl;
}

void
NeuronExtractor::addSlice(unsigned int slice) {

	_slices[slice].insert(slice);
}

void
NeuronExtractor::mergeSlices(unsigned int slice1, unsigned int slice2) {

	// make sure we have partner sets for the given slices
	addSlice(slice1);
	addSlice(slice2);

	// get all partners of slice2 and add them to the partners of slice1
	foreach (unsigned int id, _slices[slice2])
		_slices[slice1].insert(id);

	// slice1 is now the only one who knows all its partners

	// the partners of slice1 might not know about slice2 or any of its partners

	// for each partner of slice1, add all partners of slice1 to the partner list
	foreach (unsigned int id, _slices[slice1]) {

		foreach (unsigned int partner, _slices[slice1])
			_slices[id].insert(partner);
	}
}
