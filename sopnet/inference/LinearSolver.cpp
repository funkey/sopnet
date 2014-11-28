#include <boost/timer/timer.hpp>
#include <util/Logger.h>
#include <util/foreach.h>
#include <util/helpers.hpp>
#include "LinearSolver.h"

static logger::LogChannel linearsolverlog("linearsolverlog", "[LinearSolver] ");

LinearSolver::LinearSolver(const LinearSolverBackendFactory& backendFactory) :
	_solution(new Solution()),
	_objectiveDirty(true),
	_linearConstraintsDirty(true),
	_parametersDirty(true),
	_pinnedChanged(false) {

	registerInput(_objective, "objective");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_parameters, "parameters");
	registerOutput(_solution, "solution");

	// create solver backend
	_solver = backendFactory.createLinearSolverBackend();

	// register callbacks for input changes
	_objective.registerCallback(&LinearSolver::onObjectiveModified, this);
	_linearConstraints.registerCallback(&LinearSolver::onLinearConstraintsModified, this);
	_parameters.registerCallback(&LinearSolver::onParametersModified, this);
}

LinearSolver::~LinearSolver() {

	delete _solver;
}

void
LinearSolver::pinVariable(unsigned int varNum, double value) {

	// already pinned to that value?
	if (_pinned.count(varNum) && _pinned[varNum] == value)
		return;

	_pinned[varNum] = value;
	_pinnedChanged = true;
	setDirty(_solution);
}

bool
LinearSolver::unpinVariable(unsigned int varNum) {

	// was it pinned?
	if (_pinned.erase(varNum) > 0) {

		_unpinned.insert(varNum);
		_pinnedChanged = true;
		setDirty(_solution);

		return true;
	}

	return false;
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

	boost::timer::auto_cpu_timer timer("\tLinearSolver::updateOutputs()\t\t%ws\n");

	updateLinearProgram();

	solve();
}

void
LinearSolver::updateLinearProgram() {

	if (_parametersDirty) {

		LOG_DEBUG(linearsolverlog) << "initializing solver" << std::endl;

		if (_parameters.isSet())
			_solver->initialize(
					getNumVariables(),
					_parameters->getDefaultVariableType(),
					_parameters->getSpecialVariableTypes());
		else
			_solver->initialize(
					getNumVariables(),
					Continuous);

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

	if (_pinnedChanged) {

		LOG_DEBUG(linearsolverlog) << "(un)pinning variables" << std::endl;

		unsigned int varNum;
		double       value;
		foreach (boost::tie(varNum, value), _pinned)
			_solver->pinVariable(varNum, value);

		foreach (varNum, _unpinned)
			_solver->unpinVariable(varNum);
		_unpinned.clear();

		_pinnedChanged = false;
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
