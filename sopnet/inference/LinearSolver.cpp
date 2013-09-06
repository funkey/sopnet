#include <util/Logger.h>
#include <util/foreach.h>
#include <util/helpers.hpp>
#include "LinearSolver.h"

static logger::LogChannel linearsolverlog("linearsolverlog", "[LinearSolver] ");

LinearSolver::LinearSolver(const LinearSolverBackendFactory& backendFactory) :
	_objectiveDirty(true),
	_linearConstraintsDirty(true),
	_parametersDirty(true) {

	registerInput(_objective, "objective");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_parameters, "parameters");
	registerOutput(_solution, "solution");

	// create solver backend
	_solver = backendFactory.createLinearSolverBackend();

	// register callbacks for input changes
	_objective.registerBackwardCallback(&LinearSolver::onObjectiveModified, this);
	_linearConstraints.registerBackwardCallback(&LinearSolver::onLinearConstraintsModified, this);
	_parameters.registerBackwardCallback(&LinearSolver::onParametersModified, this);
}

LinearSolver::~LinearSolver() {

	delete _solver;
}

void
LinearSolver::onObjectiveModified(const pipeline::Modified&) {

	_objectiveDirty = true;
}

void
LinearSolver::onLinearConstraintsModified(const pipeline::Modified&) {

	_linearConstraintsDirty = true;
}

void
LinearSolver::onParametersModified(const pipeline::Modified&) {

	_parametersDirty = true;
}

void
LinearSolver::updateOutputs() {

	updateLinearProgram();

	solve();
}

void
LinearSolver::updateLinearProgram() {

	if (_parametersDirty) {

		LOG_DEBUG(linearsolverlog) << "initializing solver" << std::endl;

		_solver->initialize(
				getNumVariables(),
				(_parameters ? _parameters->getVariableType() : Continuous));

		_parametersDirty = false;
	}

	if (_objectiveDirty) {

		LOG_DEBUG(linearsolverlog) << "(re)setting objective" << std::endl;

		_solver->setObjective(*_objective);

		_objectiveDirty = false;
	}

	if (_linearConstraintsDirty) {

		LOG_DEBUG(linearsolverlog) << "(re)setting linear constraints" << std::endl;

		_solver->setConstraints(*_linearConstraints);

		_linearConstraintsDirty = false;
	}
}

void
LinearSolver::solve() {

	double value;

	std::string message;

	if (_solver->solve(*_solution, value, message)) {

		LOG_USER(linearsolverlog) << "optimal solution found" << std::endl;

	} else {

		LOG_ERROR(linearsolverlog) << "error: " << message << std::endl;
	}

	LOG_ALL(linearsolverlog) << "solution: " << _solution->getVector() << std::endl;
}

unsigned int
LinearSolver::getNumVariables() {

	LOG_ALL(linearsolverlog)
			<< "objective contains "
			<< _objective->getCoefficients().size()
			<< " variables" << std::endl;

	unsigned int numVars = _objective->getCoefficients().size();

	// number of vars in the constraints
	unsigned int varNum;
	double coef;
	foreach (const LinearConstraint& constraint, *_linearConstraints)
		foreach (boost::tie(varNum, coef), constraint.getCoefficients())
			numVars = std::max(numVars, varNum + 1);

	LOG_ALL(linearsolverlog)
			<< "together with the constraints, "
			<< numVars
			<< " variables are used" << std::endl;

	return numVars;
}
