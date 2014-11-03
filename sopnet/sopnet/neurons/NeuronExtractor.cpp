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

	prepareSliceMaps();

	findConnectedSlices();

	createNeuronsFromSlices();

	LOG_ALL(neuronextractorlog) << "found " << _neurons->size() << " neurons" << std::endl;
}

void
NeuronExtractor::prepareSliceMaps() {

	_sliceSegments.clear();
	_sliceNeighbors.clear();
	_unvisitedSlices.clear();

	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments()) {

		std::vector<boost::shared_ptr<Slice> > sources = segment->getSourceSlices();
		std::vector<boost::shared_ptr<Slice> > targets = segment->getTargetSlices();

		foreach (boost::shared_ptr<Slice> slice, sources) {

			_sliceSegments[slice->getId()].push_back(segment);
			_unvisitedSlices.insert(slice->getId());
		}

		foreach (boost::shared_ptr<Slice> slice, targets) {

			_sliceSegments[slice->getId()].push_back(segment);
			_unvisitedSlices.insert(slice->getId());
		}

		foreach (boost::shared_ptr<Slice> source, sources)
			foreach (boost::shared_ptr<Slice> target, targets) {

				_sliceNeighbors[source->getId()].push_back(target->getId());
				_sliceNeighbors[target->getId()].push_back(source->getId());
			}
	}
}

void
NeuronExtractor::findConnectedSlices() {

	_connectedSlices.clear();

	while (!_unvisitedSlices.empty()) {

		// create a new component
		std::set<unsigned int> connectedSlices;

		// pick a unvisited slice and add it to the stack
		unsigned int seedSlice = *(_unvisitedSlices.begin());
		_boundarySlices.push(seedSlice);

		// while there are slices on the boundary stack
		while (!_boundarySlices.empty()) {

			// get a boundary slice
			unsigned int currentSlice = _boundarySlices.top();
			_boundarySlices.pop();

			// add it to the current component
			connectedSlices.insert(currentSlice);

			// mark it as visited
			_unvisitedSlices.erase(currentSlice);

			// add all non-visted neighbors of it to the boundary stack
			foreach (unsigned int neighbor, _sliceNeighbors[currentSlice])
				if (_unvisitedSlices.count(neighbor))
					_boundarySlices.push(neighbor);
		}

		_connectedSlices.push_back(connectedSlices);
	}
}

void
NeuronExtractor::createNeuronsFromSlices() {

	_neurons->clear();

	foreach (const std::set<unsigned int>& connectedSlices, _connectedSlices) {

		boost::shared_ptr<SegmentTree> segmentTree = boost::make_shared<SegmentTree>();

		foreach (unsigned int slice, connectedSlices) {

			foreach (boost::shared_ptr<Segment> segment, _sliceSegments[slice]) {

				if (boost::shared_ptr<EndSegment> end = boost::dynamic_pointer_cast<EndSegment>(segment))
					segmentTree->add(end);
				if (boost::shared_ptr<ContinuationSegment> continuation = boost::dynamic_pointer_cast<ContinuationSegment>(segment))
					segmentTree->add(continuation);
				if (boost::shared_ptr<BranchSegment> branch = boost::dynamic_pointer_cast<BranchSegment>(segment))
					segmentTree->add(branch);
			}
		}

		_neurons->add(segmentTree);
	}
}
