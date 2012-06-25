#ifndef SOPNET_SECTION_SELECTOR_H__
#define SOPNET_SECTION_SELECTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>

class SectionSelector : public pipeline::SimpleProcessNode {

public:

	SectionSelector(int firstSection, int lastSection);

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _subStack;

	int _firstSection;
	int _lastSection;
};


#endif // SOPNET_SECTION_SELECTOR_H__

