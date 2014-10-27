/**
 * create_ted_conditions main file. Reads a label.txt and the content of several 
 * minimalImpactTEDconditions.txt to create a single ted_conditions.txt that can 
 * be used with Jonas' matlab scripts.
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

util::ProgramOption optionFlippedFile(
		util::_long_name        = "flipped",
		util::_description_text = "A file that contains flip information, one per line: [hash of flipped segment] : [list of hashes of also flipped].",
		util::_default_value    = "flipped.txt");

util::ProgramOption optionTedConditionFile(
		util::_long_name        = "out",
		util::_description_text = "The file to store the conditions in.",
		util::_default_value    = "ted_conditions.txt");

/**
 * Read the flip information of each segment from the given file.
 */
std::map<SegmentHash, std::vector<SegmentHash> >
readFlips(const std::string& filename) {

	std::map<SegmentHash, std::vector<SegmentHash> > flips;

	std::ifstream flipfile(filename.c_str());
	std::string line;

	while (std::getline(flipfile, line)) {

		// comment or empty
		if (line.find_first_of('#') == 0 || line.empty())
			continue;

		std::stringstream linestream(line);

		// read the flipped segment hash
		SegmentHash flippedSegmentHash;
		linestream >> flippedSegmentHash;

		char colon;
		linestream >> colon;

		// syntax check
		if (colon != ':')
			UTIL_THROW_EXCEPTION(
					IOError,
					"line '" << line << "' does not have valid syntax (expected ':', got '" << colon << "'");

		// read all other segment hashes
		SegmentHash hash;

		while (linestream >> hash)
			flips[flippedSegmentHash].push_back(hash);

		LOG_DEBUG(logger::out) << "segment " << flippedSegmentHash << " flipped also " << flips[flippedSegmentHash] << std::endl;
	}

	return flips;
}

/**
 * Read the variables and their corresponding segment hashes from the given 
 * file.
 */
std::vector<std::pair<SegmentHash, bool> >
readVariables(const std::string& filename) {

	std::vector<std::pair<SegmentHash, bool> > variables;

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

		variables.push_back(std::make_pair(hash, defaultValue));
	}

	return variables;
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

		// read variables and their default value
		std::vector<std::pair<SegmentHash, bool> >       variables = readVariables(optionLabels.as<std::string>());
		std::map<SegmentHash, std::vector<SegmentHash> > flips     = readFlips(optionFlippedFile.as<std::string>());

		// assemble ted_conditions.txt
		std::ofstream conditionFile(optionTedConditionFile.as<std::string>());

		// create hash to varnum mapping
		std::map<SegmentHash, unsigned int> hashToVarnum;
		for (unsigned int i = 0; i < variables.size(); i++)
			hashToVarnum[variables[i].first] = i;

		// for each variable
		for (unsigned int i = 0; i < variables.size(); i++) {

			// get all the segments that get flipped if we flip this one
			const std::vector<SegmentHash>& flipPartners = flips[variables[i].first];

			// write the varnum of the flip partners
			foreach (SegmentHash hash, flipPartners) {

				if (!hashToVarnum.count(hash))
					std::cout << "WARNING: can not find variable number for segment with hash " << hash << std::endl;
				else
					conditionFile << hashToVarnum[hash] << " ";
			}

			conditionFile << std::endl;
		}

		std::cout << "(warinings about missing segments might be due to a limited scope of labels.txt: Some variables that get flipped in the big problem are not part of the small problem.)" << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

