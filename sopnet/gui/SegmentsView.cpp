#include "SegmentsView.h"
#include <util/Logger.h>

static logger::LogChannel segmentsviewlog("segmentsviewlog", "[SegmentsView] ");

SegmentsView::SegmentsView() {

	registerInput(_segments, "segments");
	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_sizeChanged);
}

void
SegmentsView::updateOutputs() {

	LOG_DEBUG(segmentsviewlog) << "setting painter content" << std::endl;

	_painter->setSegments(_segments);

	_sizeChanged();
}
