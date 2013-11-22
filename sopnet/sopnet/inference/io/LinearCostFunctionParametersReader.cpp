#include <fstream>
#include "LinearCostFunctionParametersReader.h"

LinearCostFunctionParametersReader::LinearCostFunctionParametersReader(std::string filename) :
	_filename(filename) {

	registerOutput(_parameters, "parameters");
}

void
LinearCostFunctionParametersReader::updateOutputs() {

	std::ifstream in(_filename.c_str());

	std::vector<double> weights;

	double weight;
	while (in >> weight)
		weights.push_back(weight);

	_parameters->setWeights(weights);
}
