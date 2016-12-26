/**
 * create_cost_function main file. Reads a label.txt and several 
 * minimalImpactTEDcoefficients.txt to create a single const_function.txt that 
 * can be used with sbmrm.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <sopnet/segments/SegmentHash.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

util::ProgramOption optionLabels(
		util::_long_name        = "labels",
		util::_description_text = "The gold standard label file.",
		util::_default_value    = "labels.txt");

util::ProgramOption optionTedCoefficientFile(
		util::_long_name        = "coefficients",
		util::_description_text = "A file that contains TED coefficients, one per line: [varnum] [coeff] # [hash] [gs] [fs] [fm] [fp] [fn].",
		util::_default_value    = "ted_coefficients.txt");

util::ProgramOption optionJustCopy(
		util::_long_name        = "justCopy",
		util::_description_text = "Don't recompute any score, just take the coefficients like they are.");

util::ProgramOption optionCostFunctionFile(
		util::_long_name        = "out",
		util::_description_text = "The file to store the coefficients in.",
		util::_default_value    = "cost_function.txt");

util::ProgramOption optionWeightSplits(
		util::_long_name        = "weightSplits",
		util::_description_text = "The factor that split errors get multiplied with",
		util::_default_value    = 1);

util::ProgramOption optionWeightMerges(
		util::_long_name        = "weightMerges",
		util::_description_text = "The factor that merge errors get multiplied with",
		util::_default_value    = 1);

/**
 * Read the coefficients of each segment from the given file.
 */
std::map<SegmentHash, double>
readCoefficients(const std::string& filename) {

	std::map<SegmentHash, double> coefficients;

	std::ifstream coefsfile(filename.c_str());

	bool justCopy = optionJustCopy;

	while (coefsfile.good()) {

		std::string line;
		std::getline(coefsfile, line);

		std::stringstream linestream(line);

		std::string varname;
		linestream >> varname;

		// first line or comment or empty or constant
		if (varname == "numVar" || varname == "constant" || varname.find_first_of('#') == 0 || varname.empty())
			continue;

		double coef;
		linestream >> coef;

		char hashSymbol;
		linestream >> hashSymbol;

		if (hashSymbol != '#')
			UTIL_THROW_EXCEPTION(
					IOError,
					"line '" << line << "' does not have valid syntax (expected '#', got '" << hashSymbol << "'");

		SegmentHash hash;
		linestream >> hash;

		if (justCopy) {

			coefficients[hash] = coef;
			continue;
		}

		double gs;
		linestream >> gs;

		double fs;
		linestream >> fs;

		double fm;
		linestream >> fm;

		double fp;
		linestream >> fp;

		double fn;
		linestream >> fn;

		double weightSplits = optionWeightSplits.as<double>();
		double weightMerges = optionWeightMerges.as<double>();

		// Initial factor accounts for sign depending on if the segment is part of the goldstandard.
		coefficients[hash] = (-2*gs + 1) * ( weightSplits * (fs + fp) + weightMerges * (fm + fn) );
	}

	return coefficients;
}

/**
 * Read the variables and their corresponding segment hashes from the given 
 * file.
 */
std::vector<SegmentHash>
readVariables(const std::string& filename) {

	std::vector<SegmentHash> hashes;

	std::ifstream labelsfile(filename.c_str());

	std::string line;

	while(std::getline(labelsfile, line)) {

		std::stringstream linestream(line);

		bool defaultValue;
		linestream >> defaultValue;

		char hashSymbol;
		linestream >> hashSymbol;

		if (hashSymbol != '#')
			UTIL_THROW_EXCEPTION(
					IOError,
					"line '" << line << "' does not have valid syntax (expected '#', got '" << hashSymbol << "'");

		SegmentHash hash;
		linestream >> hash;

		hashes.push_back(hash);
	}

	return hashes;
}

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

		// read hashes and their coefs
		std::vector<SegmentHash>      hashes = readVariables(optionLabels.as<std::string>());
		std::map<SegmentHash, double> coefs  = readCoefficients(optionTedCoefficientFile.as<std::string>());

		// assemble cost_function.txt
		std::ofstream costFunctionFile(optionCostFunctionFile.as<std::string>());

		double constant = 0;
		costFunctionFile << "numVar " << hashes.size() << std::endl;
		for (unsigned int i = 0; i < hashes.size(); i++) {

			double coef = coefs[hashes[i]];

			costFunctionFile << "c" << i << " " << coef << std::endl;

			if (coef < 0)
				constant += -coef;
		}
		costFunctionFile << "constant " << constant << std::endl;

		/*********
		 * SETUP *
		 *********/

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

