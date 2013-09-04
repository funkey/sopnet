#ifndef SOPNET_CONTINUATION_SEGMENT_H__
#define SOPNET_CONTINUATION_SEGMENT_H__

#include "Segment.h"

// forward declarations
class Slice;

class ContinuationSegment : public Segment {

public:

	ContinuationSegment(
			unsigned int id,
			Direction direction,
			boost::shared_ptr<Slice> sourceSlice,
			boost::shared_ptr<Slice> targetSlice);

	boost::shared_ptr<Slice> getSourceSlice() const;

	boost::shared_ptr<Slice> getTargetSlice() const;

	std::vector<boost::shared_ptr<Slice> > getSlices() const;

private:

	boost::shared_ptr<Slice> _sourceSlice;

	boost::shared_ptr<Slice> _targetSlice;
};

#endif // SOPNET_CONTINUATION_SEGMENT_H__

