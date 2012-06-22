#ifndef SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__
#define SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__

#include <pipeline/all.h>
#include "Slices.h"
#include "Segments.h"
#include "SegmentVisitor.h"

class GroundTruthSegmentExtractor : public pipeline::SimpleProcessNode {

public:

	GroundTruthSegmentExtractor();

private:

	class SegmentSelector : public SegmentVisitor {

	public:

		SegmentSelector(
				const std::vector<boost::shared_ptr<Slice> > prevSlices,
				const std::vector<boost::shared_ptr<Slice> > nextSlices,
				boost::shared_ptr<Segments> segments);

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		const std::set<boost::shared_ptr<Slice> > getRemainingPrevSlices() { return _remainingPrevSlices; }

		const std::set<boost::shared_ptr<Slice> > getRemainingNextSlices() { return _remainingNextSlices; }

		bool selected() { return _selected; }

	private:

		// sets of slices that have not been explained so far
		std::set<boost::shared_ptr<Slice> > _remainingPrevSlices;
		std::set<boost::shared_ptr<Slice> > _remainingNextSlices;

		// true, if the last segment was accepted
		bool _selected;
	};

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

