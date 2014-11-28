#ifndef SOPNET_NEURONS_NEURON_EXTRACTOR_H__
#define SOPNET_NEURONS_NEURON_EXTRACTOR_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/segments/SegmentTrees.h>

/**
 * Given a set of segments, extracts all connected components of slices as 
 * neurons.
 */
class NeuronExtractor : public pipeline::SimpleProcessNode<> {

public:

	NeuronExtractor();

private:

	void updateOutputs();

	void prepareSliceMaps();

	void findConnectedSlices();

	void createNeuronsFromSlices();

	pipeline::Input<Segments>      _segments;
	pipeline::Output<SegmentTrees> _neurons;

	// map from slice id to all segments it is involved in
	std::map<unsigned int, std::vector<boost::shared_ptr<Segment> > > _sliceSegments;

	// map from slice id to all neighboring slice ids
	std::map<unsigned int, std::vector<unsigned int> > _sliceNeighbors;

	// set of already visited slices
	std::set<unsigned int> _unvisitedSlices;

	// ids of slices that are at the boundary of the current component
	std::stack<unsigned int> _boundarySlices;

	// list of connected slices
	std::vector<std::set<unsigned int> > _connectedSlices;
};

#endif // SOPNET_NEURONS_NEURON_EXTRACTOR_H__

