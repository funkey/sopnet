#ifndef CELLTRACKER_GUI_TRACKLETS_PAINTER_H__
#define CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

#include <gui/RecordablePainter.h>
#include <sopnet/Segments.h>
#include <sopnet/SegmentVisitor.h>

class SegmentsPainter : public gui::RecordablePainter, public SegmentVisitor {

public:

	SegmentsPainter();

	void setSegments(boost::shared_ptr<Segments> segments);

	void visit(const EndSegment& end);

	void visit(const ContinuationSegment& continuation);

	void visit(const BranchSegment& branch);

private:

	void updateRecording();

	void drawSlice(
			boost::shared_ptr<Slice> slice,
			double red, double green, double blue,
			bool leftSide, bool rightSide,
			bool surface);

	boost::shared_ptr<Segments> _segments;

	util::rect<double> _size;

	double _zScale;
};

#endif // CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

