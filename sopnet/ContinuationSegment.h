#ifndef SOPNET_CONTINUATION_SEGMENT_H__
#define SOPNET_CONTINUATION_SEGMENT_H__

#include "Segment.h"

// forward declarations
class SegmentVisitor;
class Slice;

class ContinuationSegment : public Segment {

public:

	ContinuationSegment(
			unsigned int id,
			Direction direction,
			boost::shared_ptr<Slice> sourceSlice,
			boost::shared_ptr<Slice> targetSlice);

	void accept(SegmentVisitor& visitor);

	void accept(SegmentVisitor& visitor) const;

	boost::shared_ptr<Slice> getSourceSlice() const;

	boost::shared_ptr<Slice> getTargetSlice() const;

private:

	boost::shared_ptr<Slice> _sourceSlice;

	boost::shared_ptr<Slice> _targetSlice;
};

#endif // SOPNET_CONTINUATION_SEGMENT_H__

