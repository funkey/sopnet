#include "SegmentsView.h"
#include <util/Logger.h>

static logger::LogChannel segmentsviewlog("segmentsviewlog", "[SegmentsView] ");

SegmentsView::SegmentsView(std::string name) :
		_painter(boost::make_shared<SegmentsPainter>(name)) {

	registerInput(_segments, "segments");
	registerInput(_rawSections, "raw sections", pipeline::Optional);
	registerInput(_sliceErrors, "slice errors", pipeline::Optional);

	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_sizeChanged);
	_painter.registerForwardCallback(&SegmentsView::onKeyDown, this);
}

void
SegmentsView::onKeyDown(gui::KeyDown& signal) {

	if (signal.key == gui::keys::E) {

		_painter->showEnds(true);
		_painter->showContinuations(false);
		_painter->showBranches(false);

		signal.processed = true;

		_sizeChanged();

	} else if (signal.key == gui::keys::C) {

		_painter->showEnds(false);
		_painter->showContinuations(true);
		_painter->showBranches(false);

		signal.processed = true;

		_sizeChanged();

	} else if (signal.key == gui::keys::B) {

		_painter->showEnds(false);
		_painter->showContinuations(false);
		_painter->showBranches(true);

		signal.processed = true;

		_sizeChanged();

	} else if (signal.key == gui::keys::S) {

		_painter->showEnds(true);
		_painter->showContinuations(true);
		_painter->showBranches(true);

		signal.processed = true;

		_sizeChanged();
	}
}

void
SegmentsView::updateOutputs() {

	LOG_DEBUG(segmentsviewlog) << "setting painter content" << std::endl;

	if (_rawSections)
		_painter->setImageStack(_rawSections);

	if (_sliceErrors)
		_painter->setSliceErrors(_sliceErrors);

	_painter->setSegments(_segments);

	_sizeChanged();
}
