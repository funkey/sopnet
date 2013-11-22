#ifndef SOPNET_INFERENCE_IO_LINEAR_COST_FUNCTION_PARAMETERS_READER_H__
#define SOPNET_INFERENCE_IO_LINEAR_COST_FUNCTION_PARAMETERS_READER_H__

#include <string>
#include <pipeline/SimpleProcessNode.h>
#include <sopnet/inference/LinearCostFunctionParameters.h>

class LinearCostFunctionParametersReader : public pipeline::SimpleProcessNode<> {

public:

	LinearCostFunctionParametersReader(std::string filename);

private:

	void updateOutputs();

	pipeline::Output<LinearCostFunctionParameters> _parameters;

	std::string _filename;
};

#endif // SOPNET_INFERENCE_IO_LINEAR_COST_FUNCTION_PARAMETERS_READER_H__

