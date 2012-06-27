#include "SegmentsView.h"
#include <util/Logger.h>

static logger::LogChannel segmentsviewlog("segmentsviewlog", "[SegmentsView] ");

SegmentsView::SegmentsView() {

	registerInput(_segments, "segments");
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

	_painter->setSegments(_segments);

	_sizeChanged();
}
