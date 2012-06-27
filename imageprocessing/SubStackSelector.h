#ifndef IMAGEPROCESSING_SECTION_SELECTOR_H__
#define IMAGEPROCESSING_SECTION_SELECTOR_H__

#include <pipeline/all.h>
#include "ImageStack.h"

class SubStackSelector : public pipeline::SimpleProcessNode {

public:

	SubStackSelector(int firstImage, int lastImage);

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _subStack;

	int _firstImage;
	int _lastImage;
};


#endif // IMAGEPROCESSING_SECTION_SELECTOR_H__

