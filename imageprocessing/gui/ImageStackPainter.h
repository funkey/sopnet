#ifndef IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__
#define IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__

#include <gui/Painter.h>
#include <gui/ImagePainter.h>
#include <imageprocessing/ImageStack.h>

class ImageStackPainter : public gui::Painter {

public:

	ImageStackPainter(unsigned int numImages = 1);

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

	// the image painters for the currently visible sections
	std::vector<boost::shared_ptr<gui::ImagePainter<Image> > > _imagePainters;

	// the number of images to show at the same time
	unsigned int _numImages;

	// the section to draw
	unsigned int _section;

	// the height of the images to show
	double _imageHeight;
};

#endif // IMAGEPROCESSING_GUI_IMAGE_STACK_PAINTER_H__

