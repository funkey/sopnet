#ifndef SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__
#define SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__

#include <pipeline.h>
#include "Slices.h"
#include "Segments.h"

class GroundTruthSegmentExtractor : public pipeline::SimpleProcessNode {

public:

	GroundTruthSegmentExtractor();

private:

	void updateOutputs();

	void createEnd(Direction direction, boost::shared_ptr<Slice> slice);

	void createContinuation(boost::shared_ptr<Slice> prev, boost::shared_ptr<Slice> next);

	void createBranch(Direction direction, boost::shared_ptr<Slice> source, boost::shared_ptr<Slice> target1, boost::shared_ptr<Slice> target2);

	pipeline::Input<Slices> _prevSlices;

	pipeline::Input<Slices> _nextSlices;

	pipeline::Output<Segments> _segments;

	std::map<float, std::vector<boost::shared_ptr<Slice> > > _prevSliceValues;

	std::map<float, std::vector<boost::shared_ptr<Slice> > > _nextSliceValues;

	std::set<float> _values;
};

#endif // SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__

