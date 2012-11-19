#include <boost/lexical_cast.hpp>

#include <util/Logger.h>
#include "TimeView.h"

static logger::LogChannel timeviewlog("timeviewlog", "[TimeView] ");

TimeView::TimeView() {

	registerInput(_time, "time");
	registerOutput(_textPainter, "painter");

	_time.registerBackwardCallback(&TimeView::onModified, this);

	_textPainter.registerForwardSlot(_modified);
	_textPainter.registerForwardSlot(_sizeChanged);

	_textPainter.registerForwardCallback(&TimeView::onUpdate, this);
}

TimeView::~TimeView() {

	LOG_DEBUG(timeviewlog) << "destructed" << std::endl;
}

void
TimeView::onModified(const pipeline::Modified&) {

	_dirty = true;

	_modified();
}

void
TimeView::onUpdate(const pipeline::Update&) {

	_textPainter->setText(boost::lexical_cast<std::string>(_time->seconds));

	_dirty = false;

	_sizeChanged();
}
