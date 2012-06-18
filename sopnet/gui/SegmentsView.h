#ifndef CELLTRACKER_GUI_TRACKLETS_VIEW_H__
#define CELLTRACKER_GUI_TRACKLETS_VIEW_H__

#include <pipeline.h>
#include <sopnet/Segments.h>
#include <gui/Signals.h>
#include "SegmentsPainter.h"

class SegmentsView : public pipeline::SimpleProcessNode {

public:

	SegmentsView();

private:

	void updateOutputs();

	pipeline::Input<Segments>         _segments;
	pipeline::Output<SegmentsPainter> _painter;

	signals::Slot<gui::SizeChanged>   _sizeChanged;
};

#endif // CELLTRACKER_GUI_TRACKLETS_VIEW_H__

