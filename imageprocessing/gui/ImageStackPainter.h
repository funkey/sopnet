#ifndef IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__
#define IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__

#include <gui/Painter.h>
#include <gui/ImagePainter.h>
#include <imageprocessing/ImageStack.h>

class ImageStackPainter : public gui::Painter {

public:

	ImageStackPainter();

	void setImageStack(boost::shared_ptr<ImageStack> stack);

	void setCurrentSection(unsigned int section);

	/**
	 * Overwritten from painter.
	 */
	virtual void draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

private:

	// the whole stack
	boost::shared_ptr<ImageStack> _stack;

	// the image painter for the currently visible section
	gui::ImagePainter<Image> _imagePainter;

	// the section to draw
	unsigned int _section;
};

#endif // IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__

