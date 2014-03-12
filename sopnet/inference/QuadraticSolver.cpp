#include <util/Logger.h>
#include <util/foreach.h>
#include "QuadraticSolver.h"

static logger::LogChannel quadraticsolverlog("quadraticsolverlog", "[QuadraticSolver] ");

QuadraticSolver::QuadraticSolver(const QuadraticSolverBackendFactory& factory) :
	_solution(new Solution()) {

	registerInput(_objective, "objective");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_parameters, "parameters");
	registerOutput(_solution, "solution");

	_solver = factory.createQuadraticSolverBackend();
}

QuadraticSolver::~QuadraticSolver() {

	delete _solver;
}

void
QuadraticSolver::updateOutputs() {

	updateQuadraticProgram();

	solve();
}

void
QuadraticSolver::updateQuadraticProgram() {

	if (_parameters.isSet())
		_solver->initialize(
				getNumVariables(),
				_parameters->getDefaultVariableType(),
				_parameters->getSpecialVariableTypes());
	else
		_solver->initialize(
				getNumVariables(),
				Continuous);

	_solver->setObjective(*_objective);

	_solver->setConstraints(*_linearConstraints);
}

void
QuadraticSolver::solve() {

	double value;

	std::string message;

	if (_solver->solve(*_solution, value, message)) {

		LOG_USER(quadraticsolverlog) << "optimal solution found" << std::endl;

	} else {

		LOG_ERROR(quadraticsolverlog) << "error: " << message << std::endl;
	}
}

unsigned int
QuadraticSolver::getNumVariables() {

	unsigned int numVars = 0;

	// number of vars in the objective
	numVars = std::max<unsigned int>(numVars, _objective->getCoefficients().size());

	typedef std::pair<std::pair<unsigned int, unsigned int>, double> quad_coef_type;
	foreach (const quad_coef_type& pair, _objective->getQuadraticCoefficients())
		numVars = std::max(numVars, std::max(pair.first.first + 1, pair.first.second + 1));

	// number of vars in the constraints
	typedef std::pair<unsigned int, double> lin_coef_type;
	foreach (const LinearConstraint& constraint, *_linearConstraints)
		foreach (const lin_coef_type& pair, constraint.getCoefficients())
			numVars = std::max(numVars, pair.first + 1);

	return numVars;
}
