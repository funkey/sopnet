#ifndef SOPNET_BRANCH_SEGMENT_H__
#define SOPNET_BRANCH_SEGMENT_H__

#include "Segment.h"

// forward declarations
class Slice;

class BranchSegment : public Segment {

public:

	BranchSegment(
			unsigned int id,
			Direction direction,
			boost::shared_ptr<Slice> sourceSlice,
			boost::shared_ptr<Slice> targetSlice1,
			boost::shared_ptr<Slice> targetSlice2);

	boost::shared_ptr<Slice> getSourceSlice() const;

	boost::shared_ptr<Slice> getTargetSlice1() const;

	boost::shared_ptr<Slice> getTargetSlice2() const;

private:

	boost::shared_ptr<Slice> _sourceSlice;

	boost::shared_ptr<Slice> _targetSlice1;

	boost::shared_ptr<Slice> _targetSlice2;
};

#endif // SOPNET_BRANCH_SEGMENT_H__

