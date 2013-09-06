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

	// add a slice to the same-neuron lookup table
	void addSlice(unsigned int slice);

	// merge the slices of the neurons belonging to slice1 and slice2
	void mergeSlices(unsigned int slice1, unsigned int slice2);

	pipeline::Input<Segments> _segments;
	pipeline::Output<SegmentTrees> _neurons;

	// a lookup table for slices of the same neuron
	std::map<unsigned int, std::set<unsigned int> > _slices;

	// a lookup table for neuron ids from slice ids
	std::map<unsigned int, unsigned int> _neuronIds;
};

#endif // SOPNET_NEURONS_NEURON_EXTRACTOR_H__

