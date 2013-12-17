#ifndef CELLTRACKER_GUI_TRACKLETS_VIEW_H__
#define CELLTRACKER_GUI_TRACKLETS_VIEW_H__

#include <pipeline/all.h>
#include <gui/GuiSignals.h>
#include <gui/Keys.h>
#include <gui/KeySignals.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/evaluation/SliceErrors.h>
#include <sopnet/segments/Segments.h>
#include "SegmentsPainter.h"

class SegmentsView : public pipeline::SimpleProcessNode<> {

public:

	SegmentsView(std::string name = "");

private:

	void updateOutputs();

	void onKeyDown(gui::KeyDown& signal);

	pipeline::Input<Segments>    _segments;
	pipeline::Input<ImageStack>  _rawSections;
	pipeline::Input<SliceErrors> _sliceErrors;

	pipeline::Output<SegmentsPainter> _painter;

	signals::Slot<gui::SizeChanged>   _sizeChanged;
};

#endif // CELLTRACKER_GUI_TRACKLETS_VIEW_H__

