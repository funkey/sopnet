/**
 * create_svm_training_data main file. Reads a label.txt and features.txt to 
 * create training data to be used with libsvm and compatible packages.
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

util::ProgramOption optionLabels(
		util::_long_name        = "labels",
		util::_description_text = "The gold standard label file.",
		util::_default_value    = "labels.txt");

util::ProgramOption optionFeatures(
		util::_long_name        = "features",
		util::_description_text = "A file that contains the feature vectors, one per line in labels.txt.",
		util::_default_value    = "features.txt");

util::ProgramOption optionSvmFile(
		util::_long_name        = "out",
		util::_description_text = "The file to store the svm training data in.",
		util::_default_value    = "svm_training.txt");

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

		// open svm file
		std::ofstream svmFile(optionSvmFile.as<std::string>());

		// loop through labels and features file
		std::ifstream labelsFile(optionLabels.as<std::string>());
		std::ifstream featuresFile(optionFeatures.as<std::string>());

		std::string labelLine, featureLine;

		while (std::getline(labelsFile, labelLine) && std::getline(featuresFile, featureLine)) {

			// get the label
			bool label;
			std::stringstream labelStream(labelLine);
			labelStream >> label;

			// get the features
			std::vector<double> features;
			std::stringstream featureStream(featureLine);
			double f;
			while (featureStream >> f)
				features.push_back(f);

			// write a svm training line
			svmFile << (label ? 1 : -1) << " ";
			for (unsigned int i = 0; i < features.size(); i++)
				if (features[i] != 0)
					svmFile << " " << (i+1) << ":" << features[i];
			svmFile << std::endl;
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


