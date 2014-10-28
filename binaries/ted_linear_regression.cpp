/**
 * ted_linear_regression main file. Reads a ted_conditions.txt and 
 * minimalImpactTEDcoefficients.txt to learn linear coefficients for each 
 * variable that approximate the TED.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <sopnet/segments/SegmentHash.h>
#include <inference/QuadraticSolver.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/helpers.hpp>
#include <util/foreach.h>

std::vector<double>                             tedNumbers;
std::vector<std::set<unsigned int> >            tedConditions;
std::map<unsigned int, std::set<unsigned int> > tedConditionsByVariable;

util::ProgramOption optionRegularizerWeight(
		util::_long_name        = "regularizerWeight",
		util::_description_text = "Weight of the quadratic regularizer on the linear coefficients.",
		util::_default_value    = 0);

// read the TED number for each variable that we got by flipping it
void
readTEDnumbers(std::string filename) {

	std::ifstream file(filename.c_str());

	// read the number of variables
	std::string line;

	if (!std::getline(file, line))
		UTIL_THROW_EXCEPTION(
				IOError,
				filename << " does not contain anything");

	std::cout << "processing line '" << line << "'" << std::endl;

	std::stringstream linestream(line);
	std::string numVarToken;
	unsigned int numVars;
	if (!(linestream >> numVarToken))
		UTIL_THROW_EXCEPTION(
				IOError,
				filename << " could not read 'numVar' token");

	if (numVarToken != "numVar")
		UTIL_THROW_EXCEPTION(
				IOError,
				filename << " does not contain number of variables (got '" << numVarToken << "', expected 'numVar'");

	linestream >> numVars;

	std::cout << "reading TED numbers for " << numVars << " variables" << std::endl;

	tedNumbers.resize(numVars);

	unsigned int i = 0;
	while (std::getline(file, line)) {

		if (line.empty())
			continue;

		std::stringstream linestream(line);
		std::string variableName;
		double tedNumber;

		linestream >> variableName;
		linestream >> tedNumber;

		if (variableName == "constant")
			continue;

		tedNumbers[i] = tedNumber;
		i++;
	}

	std::cout << "read " << i << " ted numbers" << std::endl;
}

// read all the flip configuration that resulted from flipping a single variable
void
readTEDconditions(std::string filename) {

	tedConditions.clear();

	std::ifstream file(filename.c_str());
	std::string line;

	unsigned int i = 0;
	while (std::getline(file, line)) {

		std::stringstream linestream(line);
		unsigned int varNum;

		std::set<unsigned int> condition;

		while (linestream >> varNum) {

			condition.insert(varNum);
			tedConditionsByVariable[varNum].insert(i);
		}

		tedConditions.push_back(condition);
		i++;
	}

	std::cout << "read " << tedConditions.size() << " conditions" << std::endl;
}

void
increase(LinearConstraint& constraint, unsigned int i) {

	const std::map<unsigned int, double>& currentCoefs = constraint.getCoefficients();

	if (!currentCoefs.count(i))
		constraint.setCoefficient(i, 1);
	else
		constraint.setCoefficient(i, currentCoefs.at(i) + 1);
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

		LOG_USER(logger::out) << "starting..." << std::endl;

		readTEDnumbers("minimalImpactTEDcoefficients.txt");
		readTEDconditions("ted_conditions.txt");

		unsigned int numVariables = tedNumbers.size();

		// create a linear solver
		pipeline::Process<QuadraticSolver> solver;

		// create an objective
		pipeline::Value<QuadraticObjective> objective(numVariables);

		double regularizerWeight = optionRegularizerWeight;

		if (regularizerWeight != 0)
			for (unsigned int i = 0; i < numVariables; i++)
				objective->setQuadraticCoefficient(i, i, regularizerWeight);

		// create a configuration
		pipeline::Value<QuadraticSolverParameters> parameters;

		// create constraints
		pipeline::Value<LinearConstraints> constraints;

		std::cout << "assembling constraints" << std::endl;

		// one constraint per variable
		for (unsigned int i = 0; i < numVariables; i++) {

			LinearConstraint constraint;
			double b = 0;

			// get all configurations in which variable i was flipped
			const std::set<unsigned int>& configurationIndices = tedConditionsByVariable[i];

			// for each of those configurations
			foreach (unsigned int c, configurationIndices) {

				// increase each coefficient a_j that corresponds to a z_j == 1
				foreach (unsigned int j, tedConditions[c])
					increase(constraint, j);

				// add the ted number of this configuration to the constant b
				b += tedNumbers[c];
			}

			constraint.setRelation(Equal);
			constraint.setValue(b);

			constraints->add(constraint);
		}

		solver->setInput("objective", objective);
		solver->setInput("linear constraints", constraints);
		solver->setInput("parameters", parameters);

		pipeline::Value<Solution> solution = solver->getOutput("solution");

		std::cout << "computing solution" << std::endl;

		// write the solution
		std::ofstream out("weights_RMITED.txt");

		out << "numVar " << numVariables << std::endl;
		double constant = 0;
		for (unsigned int i = 0; i < numVariables; i++) {

			double value = (*solution)[i];
			out << "c" << i << " " << value << std::endl;
			if (value < 0)
				constant += -value;
		}
		out << "constant " << constant << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


