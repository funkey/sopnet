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

util::ProgramOption optionNormFile(
		util::_long_name        = "outNorm",
		util::_description_text = "The file to store the feature normalisation data in.",
		util::_default_value    = "svm_normalisation.txt");

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
		LOG_USER(logger::out) << "Opening file for writing" << std::endl;
		std::ofstream svmFile(optionSvmFile.as<std::string>());

		// Read features to compute normalisation
		// After the next loop mins and maxs should contain the
		// maximums and minimums for all features.
		LOG_USER(logger::out) << "Reading features to compute minimums and maximus" << std::endl;
		std::ifstream featuresFileNorm(optionFeatures.as<std::string>());
		std::vector<double> mins(0);
		std::vector<double> maxs(0);
		std::string featureLineNorm;

		while (std::getline(featuresFileNorm, featureLineNorm)) {

			std::stringstream featureStreamNorm(featureLineNorm);
			double f;
			unsigned int featureNumber = 0;
			while (featureStreamNorm >> f) {
				// Make sure vector is large enough
				if (mins.size() <= featureNumber)
					mins.resize(featureNumber+1,0);
				if (maxs.size() <= featureNumber)
					maxs.resize(featureNumber+1,0);

				// If appropriate set new minimum and maximum
				if (f < mins[featureNumber])
					mins[featureNumber] = f;
				if (f > maxs[featureNumber])
					maxs[featureNumber] = f;

				featureNumber++;
			}
		}

		std::ofstream normFile(optionNormFile.as<std::string>());
		for (unsigned int i = 0; i < mins.size() && i < maxs.size(); i++)
			normFile << mins[i] << " " << maxs[i] << std::endl;

		LOG_USER(logger::out) << "Looping through labels and features to write out result" << std::endl;
		// loop through labels and features file
		std::ifstream labelsFile(optionLabels.as<std::string>());
		std::ifstream featuresFile(optionFeatures.as<std::string>());

		std::string labelLine, featureLine;

		while (std::getline(labelsFile, labelLine) && std::getline(featuresFile, featureLine)) {

			// get the label
			bool label;
			std::stringstream labelStream(labelLine);
			labelStream >> label;

			// get the features and normalise them using mins and maxs from above
			std::vector<double> features;
			std::stringstream featureStream(featureLine);
			double f;
			int featureNumber = 0;
			while (featureStream >> f) {
				double f_norm = (f - mins[featureNumber]) / (maxs[featureNumber] - mins[featureNumber]);
				features.push_back(f_norm);
				featureNumber++;
			}

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
