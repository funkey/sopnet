#include <config.h>

#ifdef HAVE_GUROBI

#include <sstream>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "GurobiBackend.h"

using namespace logger;

LogChannel gurobilog("gurobilog", "[GurobiBackend] ");

util::ProgramOption optionGurobiMIPGap(
		util::_module           = "inference.gurobi",
		util::_long_name        = "mipGap",
		util::_description_text = "The Gurobi relative optimality gap.",
		util::_default_value    = 0.0001);

util::ProgramOption optionGurobiMIPFocus(
		util::_module           = "inference.gurobi",
		util::_long_name        = "mipFocus",
		util::_description_text = "The Gurobi MIP focus: 0 = balanced, 1 = feasible solutions, 2 = optimal solution, 3 = bound.",
		util::_default_value    = 0);

util::ProgramOption optionGurobiNumThreads(
		util::_module           = "inference.gurobi",
		util::_long_name        = "numThreads",
		util::_description_text = "The number of threads to be used by Gurobi. The default (0) uses all available CPUs.",
		util::_default_value    = 0);


GurobiBackend::GurobiBackend() :
	_variables(0),
	_model(_env) {
}

GurobiBackend::~GurobiBackend() {

	LOG_DEBUG(gurobilog) << "destructing gurobi solver..." << std::endl;

	if (_variables)
		delete[] _variables;
}

void
GurobiBackend::initialize(
		unsigned int numVariables,
		VariableType variableType) {

	initialize(numVariables, variableType, std::map<unsigned int, VariableType>());
}

void
GurobiBackend::initialize(
		unsigned int                                numVariables,
		VariableType                                defaultVariableType,
		const std::map<unsigned int, VariableType>& specialVariableTypes) {

	if (gurobilog.getLogLevel() >= Debug)
		setVerbose(true);
	else
		setVerbose(false);

	setMIPGap(optionGurobiMIPGap);

	if (optionGurobiMIPFocus.as<unsigned int>() <= 3)
		setMIPGap(optionGurobiMIPGap);
	else
		LOG_ERROR(gurobilog) << "Invalid value for MPI focus!" << std::endl;

	setNumThreads(optionGurobiNumThreads);

	_numVariables = numVariables;

	// delete previous variables
	if (_variables)
		delete[] _variables;

	// add new variables to the model
	if (defaultVariableType == Binary) {

		LOG_DEBUG(gurobilog) << "creating " << _numVariables << " binary variables" << std::endl;

		_variables = _model.addVars(_numVariables, GRB_BINARY);

		_model.update();

	} else if (defaultVariableType == Continuous) {

		LOG_DEBUG(gurobilog) << "creating " << _numVariables << " continuous variables" << std::endl;

		_variables = _model.addVars(_numVariables, GRB_CONTINUOUS);

		_model.update();

		// remove default lower bound on variables
		for (unsigned int i = 0; i < _numVariables; i++)
			_variables[i].set(GRB_DoubleAttr_LB, -GRB_INFINITY);

	} else if (defaultVariableType == Integer) {

		LOG_DEBUG(gurobilog) << "creating " << _numVariables << " integer variables" << std::endl;

		_variables = _model.addVars(_numVariables, GRB_INTEGER);

		_model.update();

		// remove default lower bound on variables
		for (unsigned int i = 0; i < _numVariables; i++)
			_variables[i].set(GRB_DoubleAttr_LB, -GRB_INFINITY);
	}

	// handle special variable types
	unsigned int v;
	VariableType type;
	foreach (boost::tie(v, type), specialVariableTypes) {

		char t = (type == Binary ? 'B' : (type == Integer ? 'I' : 'C'));
		LOG_ALL(gurobilog) << "changing type of variable " << v << " to " << t << std::endl;
		_variables[v].set(GRB_CharAttr_VType, t);
	}

	LOG_DEBUG(gurobilog) << "creating " << _numVariables << " ceofficients" << std::endl;
}

void
GurobiBackend::setObjective(const LinearObjective& objective) {

	setObjective((QuadraticObjective)objective);
}

void
GurobiBackend::setObjective(const QuadraticObjective& objective) {

	try {

		// set sense of objective
		if (objective.getSense() == Minimize)
			_model.set(GRB_IntAttr_ModelSense, 1);
		else
			_model.set(GRB_IntAttr_ModelSense, -1);

		// set the constant value of the objective
		_objective = objective.getConstant();

		LOG_DEBUG(gurobilog) << "setting linear coefficients" << std::endl;

		_objective.addTerms(&objective.getCoefficients()[0], _variables, _numVariables);

		// set the quadratic coefficients for all pairs of variables
		LOG_DEBUG(gurobilog) << "setting quadratic coefficients" << std::endl;

		typedef std::pair<std::pair<unsigned int, unsigned int>, double> quad_coef_pair_type;
		foreach (const quad_coef_pair_type& pair, objective.getQuadraticCoefficients()) {

			const std::pair<unsigned int, unsigned int>& variables = pair.first;
			float value = pair.second;

			LOG_ALL(gurobilog) << "setting Q(" << variables.first << ", " << variables.second
			                   << ") to " << value << std::endl;

			if (value != 0)
				_objective += _variables[variables.first]*_variables[variables.second]*value;
		}

		LOG_ALL(gurobilog) << "setting the objective " << _objective << std::endl;

		_model.setObjective(_objective);

		LOG_ALL(gurobilog) << "updating the model" << std::endl;

		_model.update();

	} catch (GRBException e) {

		LOG_ERROR(gurobilog) << "error: " << e.getMessage() << endl;
	}
}

void
GurobiBackend::setConstraints(const LinearConstraints& constraints) {

	// remove previous constraints
	foreach (GRBConstr constraint, _constraints)
		_model.remove(constraint);
	_constraints.clear();

	_model.update();

	// allocate memory for new constraints
	_constraints.reserve(constraints.size());

	try {

		LOG_DEBUG(gurobilog) << "setting " << constraints.size() << " constraints" << std::endl;

		unsigned int j = 0;
		foreach (const LinearConstraint& constraint, constraints) {

			if (j > 0)
				if (j % 1000 == 0)
					LOG_ALL(gurobilog) << "" << j << " constraints set so far" << std::endl;

			// create the lhs expression
			GRBLinExpr lhsExpr;

			// set the coefficients
			typedef std::pair<unsigned int, double> pair_type;
			foreach (const pair_type& pair, constraint.getCoefficients())
				lhsExpr += pair.second*_variables[pair.first];

			// add to the model
			_constraints.push_back(
					_model.addConstr(
						lhsExpr,
						(constraint.getRelation() == LessEqual ? GRB_LESS_EQUAL :
								(constraint.getRelation() == GreaterEqual ? GRB_GREATER_EQUAL :
										GRB_EQUAL)),
						constraint.getValue()));

			j++;
		}

		_model.update();

	} catch (GRBException e) {

		LOG_ERROR(gurobilog) << "error: " << e.getMessage() << endl;
	}
}

void
GurobiBackend::pinVariable(unsigned int varNum, double value) {

	_variables[varNum].set(GRB_DoubleAttr_LB, value);
	_variables[varNum].set(GRB_DoubleAttr_UB, value);
}

bool
GurobiBackend::unpinVariable(unsigned int varNum) {

	bool pinned = (_variables[varNum].get(GRB_DoubleAttr_LB) > -GRB_INFINITY);

	_variables[varNum].set(GRB_DoubleAttr_LB, -GRB_INFINITY);
	_variables[varNum].set(GRB_DoubleAttr_UB,  GRB_INFINITY);

	return pinned;
}

bool
GurobiBackend::solve(Solution& x, double& value, std::string& msg) {

	try {

		LOG_ALL(gurobilog) << "solving model " << _model.getObjective() << std::endl;

		_model.optimize();

		int status = _model.get(GRB_IntAttr_Status);

		if (status != GRB_OPTIMAL) {
			msg = "Optimal solution *NOT* found";
			return false;
		} else
			msg = "Optimal solution found";

		// extract solution

		x.resize(_numVariables);
		for (unsigned int i = 0; i < _numVariables; i++)
			x[i] = _variables[i].get(GRB_DoubleAttr_X);

		// get current value of the objective
		value = _model.get(GRB_DoubleAttr_ObjVal);

	} catch (GRBException e) {

		LOG_ERROR(gurobilog) << "error: " << e.getMessage() << endl;

		msg = e.getMessage();

		return false;
	}

	return true;
}

void
GurobiBackend::setMIPGap(double gap) {

	_model.getEnv().set(GRB_DoubleParam_MIPGap, gap);
}

void
GurobiBackend::setMIPFocus(unsigned int focus) {

	_model.getEnv().set(GRB_IntParam_MIPFocus, focus);
}

void
GurobiBackend::setNumThreads(unsigned int numThreads) {

	_model.getEnv().set(GRB_IntParam_Threads, numThreads);
}

void
GurobiBackend::setVerbose(bool verbose) {

	// setup GRB environment
	if (verbose)
		_model.getEnv().set(GRB_IntParam_OutputFlag, 1);
	else
		_model.getEnv().set(GRB_IntParam_OutputFlag, 0);
}

void
GurobiBackend::dumpProblem(std::string filename) {

	try {

		_model.write(filename);

		LOG_ALL(gurobilog) << "model dumped to "
						   << filename << std::endl;

		LOG_ALL(gurobilog) << _model.getObjective() << std::endl;

	} catch (GRBException e) {

		LOG_ERROR(gurobilog) << "error: " << e.getMessage() << endl;
	}
}

#endif // HAVE_GUROBI
