#include <fstream>
#include <util/Logger.h>

#include "SolutionReader.h"

logger::LogChannel solutionreaderlog("solutionreaderlog", "[SolutionReader] ");

void
SolutionReader::updateOutputs() {

	std::ifstream in(_filename.c_str());
	std::map<unsigned int, int> solution;

	int maxId = -1;

	while (in) {

		unsigned int variableId;
		std::string  value;

		in >> variableId;
		in >> value;

		if (!in)
			break;

		LOG_ALL(solutionreaderlog) << "read variable " << variableId << " with value " << value << std::endl;

		if (value == "?") {

			solution[variableId] = -1;
			LOG_ALL(solutionreaderlog) << "\tthis is an unknown value" << std::endl;

		} else {

			solution[variableId] = boost::lexical_cast<int>(value);
		}

		if (maxId < (int)variableId)
			maxId = variableId;
	}

	LOG_DEBUG(solutionreaderlog) << "read " << (maxId + 1) << " variables" << std::endl;

	*_solution = Solution(maxId + 1);

	unsigned int variableId;
	int value;
	foreach (boost::tie(variableId, value), solution)
		(*_solution)[variableId] = value;
}
