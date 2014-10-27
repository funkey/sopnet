/**
 * create_weightvector_from_SvmOutput main file. Reads a model.dat to 
 * create a weight vector to be used with sopnet.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <sopnet/segments/SegmentHash.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/helpers.hpp>
#include <util/foreach.h>

util::ProgramOption optionModelFile(
		util::_long_name        = "model",
		util::_description_text = "A file that contains the svm model",
		util::_default_value    = "model.dat");

util::ProgramOption optionFeatureWeightsFile(
		util::_long_name        = "out",
		util::_description_text = "The file to store the weight vector in.",
		util::_default_value    = "feature_weights.dat");

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		logger::LogManager::init();

		LOG_USER(logger::out) << "[main] starting..." << std::endl;

		// open feature weights file
		std::ofstream featureWeightsFile(optionFeatureWeightsFile.as<std::string>());

		// open models file
		std::ifstream modelFile(optionModelFile.as<std::string>());

		double threshold;
		std::vector<double> weights;

		std::string line;
		// Read file, line by line
		while (std::getline(modelFile, line)) {

			// Skip if not at threshold line
			if (line.find("threshold") == std::string::npos) {
				continue;
			}

			// get threshold
			std::stringstream thresholdStream(line);
			thresholdStream >> threshold;

			// The next line contains the weight vector
			std::getline(modelFile, line);
			std::stringstream weightLine(line);

			// The first element of that line is alpha times y
			double alphay;
			weightLine >> alphay;
			if (alphay != 1)
				LOG_USER(logger::out) << "Alpha times y was expected to be 1, but was not." << std::endl;

			std::string indexAndWeight;
			int vector_index = 0;
			while (weightLine >> indexAndWeight && indexAndWeight != "#"){
				//LOG_USER(logger::out) << "indexAndWeight: " << indexAndWeight << std::endl;
				// Get index and weight
				std::string::size_type n;
				n = indexAndWeight.find(":");
				std::string indexStr = indexAndWeight.substr(0,n);
				std::string weightStr = indexAndWeight.substr(n+1,indexAndWeight.size()-n);
				//LOG_USER(logger::out) << "indexStr: " << indexStr << std::endl;
				//LOG_USER(logger::out) << "weightStr: " << weightStr << std::endl;
				int index = std::stoi( indexStr );
				double weight = std::stod( weightStr );
				while (vector_index < index -1) {
					weights.push_back(0);
					vector_index++;
				}
				weights.push_back(weight);
				vector_index++;
			}
		}

		// Write out weights
		for (unsigned int i = 0; i < weights.size(); i++) {
			featureWeightsFile << weights[i] << std::endl;
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


