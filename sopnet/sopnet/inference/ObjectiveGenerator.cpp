#include <util/foreach.h>
#include "ObjectiveGenerator.h"

static logger::LogChannel objectivegeneratorlog("objectivegeneratorlog", "[ObjectiveGenerator] ");

ObjectiveGenerator::ObjectiveGenerator() :
	_objective(new LinearObjective()) {

	registerInput(_segments, "segments");
	registerInputs(_costFunctions, "cost functions");

	registerOutput(_objective, "objective");
}

void
ObjectiveGenerator::updateOutputs() {

	updateObjective();
}

void
ObjectiveGenerator::updateObjective() {

	LOG_DEBUG(objectivegeneratorlog) << "updating the objective" << std::endl;

	// we have as many linear coefficients as segments
	_objective->resize(_segments->size());

	// create a vector of all accumulated costs
	std::vector<double> allCosts(_segments->size(), 0);

	// accumulate costs
	for (unsigned int i = 0; i < _costFunctions.size(); i++) {

		costs_function_type& costFunction = *_costFunctions[i];
		costFunction(_segments->getEnds(), _segments->getContinuations(), _segments->getBranches(), allCosts);
	}

	// end segments at the beginning and end of the stack should come for free
	// (this is assuming that the end segments have the first entries in the 
	// costs vector)
	unsigned int i = 0;
	unsigned int numInterSectionIntervals = _segments->getNumInterSectionIntervals();
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		if (end->getInterSectionInterval() == 0 || end->getInterSectionInterval() == numInterSectionIntervals - 1)
			allCosts[i] = 0;

		i++;
	}

	// set the coefficients
	for (unsigned int i = 0; i < allCosts.size(); i++)
		_objective->setCoefficient(i, allCosts[i]);
}
