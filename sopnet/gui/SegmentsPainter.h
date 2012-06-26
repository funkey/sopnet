#ifndef CELLTRACKER_GUI_TRACKLETS_PAINTER_H__
#define CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

#include <gui/RecordablePainter.h>
#include <sopnet/Segments.h>

class SegmentsPainter : public gui::RecordablePainter {

public:

	SegmentsPainter();

	void setSegments(boost::shared_ptr<Segments> segments);

private:

	void updateRecording();

	void draw(const EndSegment& end);

	void draw(const ContinuationSegment& continuation);

	void draw(const BranchSegment& branch);

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

