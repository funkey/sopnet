#ifndef SOPNET_EVALUATION_VARIATION_OF_INFORMATION_H__
#define SOPNET_EVALUATION_VARIATION_OF_INFORMATION_H__

#include <pipeline/all.h>

#include <imageprocessing/ImageStack.h>

class VariationOfInformation : public pipeline::SimpleProcessNode<> {

	typedef std::map<float, double>                   LabelProb;
	typedef std::map<std::pair<float, float>, double> JointLabelProb;

public:

	VariationOfInformation();

private:

	void updateOutputs();

	// input image stacks
	pipeline::Input<ImageStack> _stack1;
	pipeline::Input<ImageStack> _stack2;

	// variation of information
	pipeline::Output<double>    _variationOfInformation;

	// label probabilities
	LabelProb      _p1;
	LabelProb      _p2;
	JointLabelProb _p12;
};

#endif // SOPNET_EVALUATION_VARIATION_OF_INFORMATION_H__

