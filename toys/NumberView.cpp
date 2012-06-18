#include <string>

#include <boost/lexical_cast.hpp>

#include "NumberView.h"

NumberView::NumberView() {

	registerInput(_value, "value");
	registerOutput(_painter, "painter");

	_value.registerBackwardCallback(&NumberView::onModified, this);
	_painter.registerForwardCallback(&NumberView::onUpdate, this);
	_painter.registerForwardSlot(_modified);
	_painter.registerForwardSlot(_sizeChanged);
}

void
NumberView::onModified(const pipeline::Modified& signal) {

	_dirty = true;
	_modified();
}

void
NumberView::onUpdate(const pipeline::Update& signal) {

	if (_dirty) {

		_painter->setText(boost::lexical_cast<std::string>(*_value));
		_dirty = false;

		_sizeChanged();
	}
}
