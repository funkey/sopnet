#include "ImageStackView.h"
#include <util/Logger.h>

static logger::LogChannel imagestackviewlog("imagestackviewlog", "[ImageStackView] ");

ImageStackView::ImageStackView(unsigned int numImages) :
	_painter(boost::make_shared<ImageStackPainter>(numImages)),
	_section(0),
	_currentImage(boost::make_shared<Image>()) {

	registerInput(_stack, "imagestack");
	registerOutput(_painter, "painter");
	registerOutput(_currentImage, "current image");

	_painter.registerForwardSlot(_sizeChanged);
	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardCallback(&ImageStackView::onKeyDown, this);
}

void
ImageStackView::updateOutputs() {

	util::rect<double> oldSize = _painter->getSize();

	_painter->setImageStack(_stack);

	util::rect<double> newSize = _painter->getSize();

	if (oldSize == newSize) {

		LOG_ALL(imagestackviewlog) << "image size did not change -- sending ContentChanged" << std::endl;

		_contentChanged();

	} else {

		LOG_ALL(imagestackviewlog) << "image size did change -- sending SizeChanged" << std::endl;

		_sizeChanged();
	}

	if (_stack->size() > _section)
		*_currentImage = *(*_stack)[_section];
}

void
ImageStackView::onKeyDown(gui::KeyDown& signal) {

	LOG_ALL(imagestackviewlog) << "got a key down event" << std::endl;

	if (signal.key == gui::keys::A) {

		_section = std::max(0, _section - 1);

		LOG_ALL(imagestackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
		setDirty(_currentImage);
	}

	if (signal.key == gui::keys::D) {

		_section = std::min((int)_stack->size() - 1, _section + 1);

		LOG_ALL(imagestackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
		setDirty(_currentImage);
	}
}

