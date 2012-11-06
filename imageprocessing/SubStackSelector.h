#ifndef IMAGEPROCESSING_SECTION_SELECTOR_H__
#define IMAGEPROCESSING_SECTION_SELECTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>

class SubStackSelector : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a new sub stack selector.
	 *
	 * @param firstImage The first image to use.
	 * @param lastImage The last image to use. If this number is less or equal
	 *                  zero, the last |lastImage| images will be excluded.
	 */
	SubStackSelector(int firstImage, int lastImage);

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _subStack;

	int _firstImage;
	int _lastImage;
};


#endif // IMAGEPROCESSING_SECTION_SELECTOR_H__

