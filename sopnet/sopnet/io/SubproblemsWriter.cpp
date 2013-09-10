#include <fstream>
#include <util/exceptions.h>

#include "SubproblemsWriter.h"

SubproblemsWriter::SubproblemsWriter(std::string& filename) :
	_filename(filename) {

	registerInput(_subproblems, "subproblems");
}

void
SubproblemsWriter::write(std::string filename) {

	if (filename == "")
		filename = _filename;

	std::ofstream out(filename.c_str());

	boost::shared_ptr<Problem> problem = _subproblems->getProblem();

	unsigned int numVars = problem->getObjective()->size();
	unsigned int numConstraints = problem->getLinearConstraints()->size();
	unsigned int numFunctions = numVars + numConstraints;

	//////////////////////
	// dump the problem //
	//////////////////////

	// problem size
	out << "# variables functions factors" << std::endl;
	out << numVars << " " << numFunctions << " " << numFunctions << std::endl;
	out << std::endl;

	// all variables are binary
	for (unsigned int i = 0; i < numVars; i++)
		out << "2" << std::endl;

	// list of functions

	// unaries
	for (unsigned int i = 0; i < numVars; i++)
		out << "table 1 2 0 " << problem->getObjective()->getCoefficients()[i] << std::endl;
	// constraints
	foreach (LinearConstraint& constraint, *problem->getLinearConstraints()) {

		out << "constraint " << constraint.getCoefficients().size();
		unsigned int var;
		double coef;
		foreach (boost::tie(var, coef), constraint.getCoefficients())
			out << " " << coef;

		switch (constraint.getRelation()) {

			case LessEqual:
				out << " <= ";
				break;
			case GreaterEqual:
				out << " >= ";
				break;
			case Equal:
				out << " == ";
				break;
			default:
				BOOST_THROW_EXCEPTION(Exception() << error_message("unknown relation") << STACK_TRACE);
		}

		out << constraint.getValue() << std::endl;
	}

	// list of factors with regions

	// unaries
	for (unsigned int i = 0; i < numVars; i++) {

		out << i /* function num */ << " " << i /* variable num */;

		// regions are subproblem ids
		foreach (unsigned int subproblem, _subproblems->getVariableSubproblems(i))
			out << " " << subproblem;

		out << std::endl;
	}

	// constraints
	for (unsigned int i = 0; i < numConstraints; i++) {

		LinearConstraint& constraint = (*problem->getLinearConstraints())[i];

		out << (i+numVars) /* function num */;
		unsigned int var;
		double coef;
		foreach (boost::tie(var, coef), constraint.getCoefficients())
			out << " " << var /* variable num */;

		// regions are subproblem ids
		foreach (unsigned int subproblem, _subproblems->getConstraintSubproblems(i))
			out << " " << subproblem;

		out << std::endl;
	}
}

