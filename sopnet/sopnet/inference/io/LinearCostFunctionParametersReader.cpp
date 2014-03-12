#include <fstream>
#include "LinearCostFunctionParametersReader.h"

LinearCostFunctionParametersReader::LinearCostFunctionParametersReader(std::string filename) :
	_parameters(new LinearCostFunctionParameters()),
	_filename(filename) {

	registerInput(_fileContent, "file content", pipeline::Optional);
	registerOutput(_parameters, "parameters");
}

void
LinearCostFunctionParametersReader::updateOutputs() {

	boost::shared_ptr<std::ifstream> in;

	if (_fileContent.isSet()) {

		in = _fileContent.getSharedPointer();

	} else {

		in = boost::make_shared<std::ifstream>(_filename.c_str());
	}

	// reset input stream to beginning of file
	in->clear();
	in->seekg(0);

	std::vector<double> weights;
	double weight;
	while (*in >> weight)
		weights.push_back(weight);

	_parameters->setWeights(weights);
}
