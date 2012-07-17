#include "SegmentsStackView.h"
#include <util/Logger.h>

static logger::LogChannel segmentsstackviewlog("segmentsstackviewlog", "[SegmentsStackView] ");

SegmentsStackView::SegmentsStackView() :
	_section(0) {

	registerInput(_segments, "segments");
	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_sizeChanged);
	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardCallback(&SegmentsStackView::onKeyDown, this);
}

void
SegmentsStackView::updateOutputs() {

	util::rect<double> oldSize = _painter->getSize();

	_painter->setSegments(_segments);

	util::rect<double> newSize = _painter->getSize();

	if (oldSize == newSize) {

		LOG_ALL(segmentsstackviewlog) << "segments size did not change -- sending ContentChanged" << std::endl;

		_contentChanged();

	} else {

		LOG_ALL(segmentsstackviewlog) << "segments size did change -- sending SizeChanged" << std::endl;

		_sizeChanged();
	}
}

void
SegmentsStackView::onKeyDown(gui::KeyDown& signal) {

	LOG_ALL(segmentsstackviewlog) << "got a key down event" << std::endl;

	if (signal.key == gui::keys::A) {

		_section = std::max(0, _section - 1);

		LOG_ALL(segmentsstackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::D) {

		_section = std::min((int)_segments->getNumInterSectionIntervals() - 2, _section + 1);

		LOG_ALL(segmentsstackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
	}
}

