#ifndef SOPNET_SEGMENT_VISITOR_H__
#define SOPNET_SEGMENT_VISITOR_H__

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class SegmentVisitor {

public:

	virtual void visit(const EndSegment& end) = 0;

	virtual void visit(const ContinuationSegment& continuation) = 0;

	virtual void visit(const BranchSegment& branch) = 0;
};

#endif // SOPNET_SEGMENT_VISITOR_H__

