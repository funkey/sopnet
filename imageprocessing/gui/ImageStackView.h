#ifndef IMAGEPROCESSING_GUI_IMAGE_STACK_VIEW_H__
#define IMAGEPROCESSING_GUI_IMAGE_STACK_VIEW_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <gui/Keys.h>
#include <gui/Signals.h>
#include "ImageStackPainter.h"

class ImageStackView : public pipeline::SimpleProcessNode<> {

public:

	ImageStackView(unsigned int numImages = 1);

private:

	void updateOutputs();

	void onKeyDown(gui::KeyDown& signal);

	pipeline::Input<ImageStack>         _stack;
	pipeline::Output<ImageStackPainter> _painter;
	pipeline::Output<Image>             _currentImage;

	signals::Slot<gui::SizeChanged>    _sizeChanged;
	signals::Slot<gui::ContentChanged> _contentChanged;

	// the section to show
	int _section;
};

#endif // IMAGEPROCESSING_GUI_IMAGE_STACK_VIEW_H__

