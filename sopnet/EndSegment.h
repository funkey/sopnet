#ifndef SOPNET_END_SEGMENT_H__
#define SOPNET_END_SEGMENT_H__

#include "Segment.h"

// forward declarations
class SegmentVisitor;
class Slice;

class EndSegment : public Segment {

public:

	EndSegment(
			unsigned int id,
			Direction direction,
			boost::shared_ptr<Slice> slice);

	void accept(SegmentVisitor& visitor);

	void accept(SegmentVisitor& visitor) const;

	boost::shared_ptr<Slice> getSlice() const;

private:

	boost::shared_ptr<Slice> _slice;
};

#endif // SOPNET_END_SEGMENT_H__

