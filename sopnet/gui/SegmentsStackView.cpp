#include "SegmentsStackView.h"
#include <util/Logger.h>

static logger::LogChannel segmentsstackviewlog("segmentsstackviewlog", "[SegmentsStackView] ");

SegmentsStackView::SegmentsStackView(bool onlyOneSegment) :
	_painter(boost::make_shared<SegmentsStackPainter>(onlyOneSegment)),
	_section(0),
	_segmentsModified(true) {

	registerInput(_segments, "segments");
	registerOutput(_painter, "painter");

	_segments.registerBackwardCallback(&SegmentsStackView::onSegmentsModified, this);

	_painter.registerForwardSlot(_sizeChanged);
	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardCallback(&SegmentsStackView::onKeyDown, this);
	_painter.registerForwardCallback(&SegmentsStackView::onMouseDown, this);
}

void
SegmentsStackView::onSegmentsModified(const pipeline::Modified&) {

	_segmentsModified = true;
}

void
SegmentsStackView::updateOutputs() {

	util::rect<double> oldSize = _painter->getSize();

	if (_segmentsModified) {

		_painter->setSegments(_segments);

		_segmentsModified = false;
	}

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

	if (signal.key == gui::keys::O) {

		_painter->toggleShowOnlyOneSegment();

		setDirty(_painter);
	}

	if (signal.key == gui::keys::E) {

		_painter->showEnds(true);
		_painter->showContinuations(false);
		_painter->showBranches(false);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::C) {

		_painter->showEnds(false);
		_painter->showContinuations(true);
		_painter->showBranches(false);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::B) {

		_painter->showEnds(false);
		_painter->showContinuations(false);
		_painter->showBranches(true);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::S) {

		_painter->showEnds(true);
		_painter->showContinuations(true);
		_painter->showBranches(true);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::N) {

		_painter->showSliceIds(signal.modifiers & gui::keys::ShiftDown);
	}
}

void
SegmentsStackView::onMouseDown(gui::MouseDown& signal) {

	if (_painter->onlyOneSegment()) {

		if (signal.button == gui::buttons::Left)
			_painter->setFocus(signal.position);

		if (signal.button == gui::buttons::WheelDown)
			_painter->nextSegment();

		if (signal.button == gui::buttons::WheelUp)
			_painter->prevSegment();

		setDirty(_painter);
	}
}
