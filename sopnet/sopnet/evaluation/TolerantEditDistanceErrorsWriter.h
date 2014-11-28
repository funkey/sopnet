#ifndef SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_WRITER_H__
#define SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_WRITER_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include "TolerantEditDistanceErrors.h"

/**
 * Takes TED errors and the ground truth and reconstruction image stacks to 
 * produce one image stack for every split and merge error.
 */
class TolerantEditDistanceErrorsWriter : public pipeline::SimpleProcessNode<> {

public:

	TolerantEditDistanceErrorsWriter();

	/**
	 * Write the split and merge images to the given directory.
	 */
	void write(const std::string& baseDirectory);

private:

	void updateOutputs() {}

	// write a stack with only the given label
	void writeStack(
			const std::string&  dirName,
			const std::string&  stackName,
			ImageStack&         stack,
			float               label);

	// write a stack with only the given labels
	void writeStack(
			const std::string&     dirName,
			const std::string&     stackName,
			ImageStack&            stack,
			const std::set<float>& labels);

	pipeline::Input<ImageStack>                 _groundTruth;
	pipeline::Input<ImageStack>                 _reconstruction;
	pipeline::Input<TolerantEditDistanceErrors> _tedErrors;
};

#endif // SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_WRITER_H__

