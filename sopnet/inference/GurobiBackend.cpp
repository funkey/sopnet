#include <config.h>

#ifdef HAVE_GUROBI

#include <sstream>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "GurobiBackend.h"

#define GRB_CHECK(call) \
		grbCheck(#call, __FILE__, __LINE__, call)

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

util::ProgramOption optionGurobiTimeout(
		util::_module           = "inference.gurobi",
		util::_long_name        = "timeout",
		util::_description_text = "The number of seconds after which to stop the gurobi solver and report a sub-optimal solution.",
		util::_default_value    = 0);

util::ProgramOption optionGurobiNumThreads(
		util::_module           = "inference.gurobi",
		util::_long_name        = "numThreads",
		util::_description_text = "The number of threads to be used by Gurobi. The default (0) uses all available CPUs.",
		util::_default_value    = 0);

util::ProgramOption optionGurobiDumpIlp(
		util::_module           = "inference.gurobi",
		util::_long_name        = "dumpILP",
		util::_description_text = "Write the ILP into a file.");

GurobiBackend::GurobiBackend() :
	_numVariables(0),
	_numConstraints(0),
	_env(0),
	_model(0) {

	GRB_CHECK(GRBloadenv(&_env, NULL));
}

GurobiBackend::~GurobiBackend() {

	LOG_DEBUG(gurobilog) << "destructing gurobi solver..." << std::endl;

	if (_model)
		GRBfreemodel(_model);

	if (_env)
		GRBfreeenv(_env);
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

	// create a new model

	if (_model)
		GRBfreemodel(_model);
	GRB_CHECK(GRBnewmodel(_env, &_model, NULL, 0, NULL, NULL, NULL, NULL, NULL));

	// set parameters

	if (gurobilog.getLogLevel() >= Debug)
		setVerbose(true);
	else
		setVerbose(false);

	setMIPGap(optionGurobiMIPGap);

	if (optionGurobiMIPFocus.as<unsigned int>() <= 3)
		setMIPFocus(optionGurobiMIPFocus.as<unsigned int>());
	else
		LOG_ERROR(gurobilog) << "Invalid value for MPI focus!" << std::endl;

	if (optionGurobiTimeout)
		setTimeout(optionGurobiTimeout.as<double>());

	setNumThreads(optionGurobiNumThreads);

	// add new variables to the model

	_numVariables = numVariables;

	// create arrays of  variable types and infinite lower bounds
	char* vtypes = new char[_numVariables];
	double* lbs = new double[_numVariables];
	for (int i = 0; i < _numVariables; i++) {

		VariableType type = defaultVariableType;
		if (specialVariableTypes.count(i))
			type = specialVariableTypes.at(i);
		char t = (type == Binary ? 'B' : (type == Integer ? 'I' : 'C'));

		vtypes[i] = t;
		lbs[i] = -GRB_INFINITY;
	}

	LOG_DEBUG(gurobilog) << "creating " << _numVariables << " variables" << std::endl;

	GRB_CHECK(GRBaddvars(
			_model,
			_numVariables,
			0,                // num non-zeros for constraint matrix (we set it later)
			NULL, NULL, NULL, // vbeg, vind, vval for constraint matrix
			NULL,             // obj (we set it later)
			lbs, NULL,        // lower and upper bound, set to -inf and inf
			vtypes,           // variable types
			NULL));           // names

	GRB_CHECK(GRBupdatemodel(_model));

	delete[] vtypes;
	delete[] lbs;
}

void
GurobiBackend::setObjective(const LinearObjective& objective) {

	setObjective((QuadraticObjective)objective);
}

void
GurobiBackend::setObjective(const QuadraticObjective& objective) {

	// set sense of objective
	if (objective.getSense() == Minimize) {
		GRB_CHECK(GRBsetintattr(_model, GRB_INT_ATTR_MODELSENSE, +1));
	} else {
		GRB_CHECK(GRBsetintattr(_model, GRB_INT_ATTR_MODELSENSE, -1));
	}

	// set the constant value of the objective
	GRB_CHECK(GRBsetdblattr(_model, GRB_DBL_ATTR_OBJCON, objective.getConstant()));

	LOG_DEBUG(gurobilog) << "setting linear coefficients" << std::endl;

	GRB_CHECK(GRBsetdblattrarray(
			_model,
			GRB_DBL_ATTR_OBJ,
			0 /* start */, _numVariables,
			const_cast<double*>(&objective.getCoefficients()[0])));

	// remove all previous quadratic terms
	GRB_CHECK(GRBdelq(_model));

	// set the quadratic coefficients for all pairs of variables
	LOG_DEBUG(gurobilog) << "setting quadratic coefficients" << std::endl;

	for (auto& pair : objective.getQuadraticCoefficients()) {

		const std::pair<unsigned int, unsigned int>& variables = pair.first;
		float value = pair.second;

		LOG_ALL(gurobilog) << "setting Q(" << variables.first << ", " << variables.second
						   << ") to " << value << std::endl;

		if (value != 0) {

			int row = variables.first;
			int col = variables.second;
			double val = value;
			GRB_CHECK(GRBaddqpterms(_model, 1, &row, &col, &val));
		}
	}

	LOG_ALL(gurobilog) << "updating the model" << std::endl;

	GRB_CHECK(GRBupdatemodel(_model));
}

void
GurobiBackend::setConstraints(const LinearConstraints& constraints) {

	// delete all previous constraints

	if (_numConstraints > 0) {

		int* constraintIndicies = new int[_numConstraints];
		for (int i = 0; i < _numConstraints; i++)
			constraintIndicies[i] = i;
		GRB_CHECK(GRBdelconstrs(_model, _numConstraints, constraintIndicies));

		GRB_CHECK(GRBupdatemodel(_model));
	}

	LOG_DEBUG(gurobilog) << "setting " << constraints.size() << " constraints" << std::endl;

	_numConstraints = constraints.size();
	unsigned int j = 0;
	for (const LinearConstraint& constraint : constraints) {

		if (j > 0)
			if (j % 1000 == 0)
				LOG_ALL(gurobilog) << "" << j << " constraints set so far" << std::endl;

		addConstraint(constraint);

		j++;
	}

	GRB_CHECK(GRBupdatemodel(_model));
}

void
GurobiBackend::addConstraint(const LinearConstraint& constraint) {

	int numNz = constraint.getCoefficients().size();

	int*    inds = new int[numNz];
	double* vals = new double[numNz];

	// set the coefficients
	int i = 0;
	for (auto& pair : constraint.getCoefficients()) {

		inds[i] = pair.first;
		vals[i] = pair.second;
		i++;
	}

	GRB_CHECK(GRBaddconstr(
			_model,
			numNz,
			inds,
			vals,
			(constraint.getRelation() == LessEqual ? GRB_LESS_EQUAL :
					(constraint.getRelation() == GreaterEqual ? GRB_GREATER_EQUAL :
							GRB_EQUAL)),
			constraint.getValue(),
			NULL /* optional name */));

	delete[] inds;
	delete[] vals;
}

void
GurobiBackend::pinVariable(unsigned int varNum, double value) {

	GRB_CHECK(GRBsetdblattrelement(_model, GRB_DBL_ATTR_LB, varNum, value));
	GRB_CHECK(GRBsetdblattrelement(_model, GRB_DBL_ATTR_UB, varNum, value));
}

bool
GurobiBackend::unpinVariable(unsigned int varNum) {

	double lb;
	GRB_CHECK(GRBgetdblattrelement(_model, GRB_DBL_ATTR_LB, varNum, &lb));
	bool pinned = (lb > -GRB_INFINITY);

	GRB_CHECK(GRBsetdblattrelement(_model, GRB_DBL_ATTR_LB, varNum, -GRB_INFINITY));
	GRB_CHECK(GRBsetdblattrelement(_model, GRB_DBL_ATTR_UB, varNum,  GRB_INFINITY));

	return pinned;
}

bool
GurobiBackend::solve(Solution& x, double& value, std::string& msg) {

	if (optionGurobiDumpIlp)
		dumpProblem(optionGurobiDumpIlp);

	GRB_CHECK(GRBupdatemodel(_model));

	GRB_CHECK(GRBoptimize(_model));

	int status;
	GRB_CHECK(GRBgetintattr(_model, GRB_INT_ATTR_STATUS, &status));

	if (status != GRB_OPTIMAL) {

		msg = "Optimal solution *NOT* found";

		// see if a feasible solution exists

		if (status == GRB_TIME_LIMIT) {

			msg += " (timeout";

			int numSolutions;
			GRB_CHECK(GRBgetintattr(_model, GRB_INT_ATTR_SOLCOUNT, &numSolutions));

			if (numSolutions == 0) {

				msg += ", no feasible solution found)";
				return false;
			}

			msg += ", " + boost::lexical_cast<std::string>(numSolutions) + " feasible solutions found)";

		} else if (status == GRB_SUBOPTIMAL) {

			msg += " (suboptimal solution found)";

		} else {

			return false;
		}

	} else {

		msg = "Optimal solution found";
	}

	// extract solution

	LOG_ALL(gurobilog) << "extracting solution for " << _numVariables << " variables" << std::endl;

	x.resize(_numVariables);
	for (unsigned int i = 0; i < _numVariables; i++)
		// in case of several suboptimal solutions, the best-objective solution 
		// is read
		GRB_CHECK(GRBgetdblattrelement(_model, GRB_DBL_ATTR_X, i, &x[i]));

	// get current value of the objective
	GRB_CHECK(GRBgetdblattr(_model, GRB_DBL_ATTR_OBJVAL, &value));

	return true;
}

void
GurobiBackend::setMIPGap(double gap) {

	GRBenv* modelenv = GRBgetenv(_model);
	GRB_CHECK(GRBsetdblparam(modelenv, GRB_DBL_PAR_MIPGAP, gap));
}

void
GurobiBackend::setMIPFocus(unsigned int focus) {

	GRBenv* modelenv = GRBgetenv(_model);
	GRB_CHECK(GRBsetintparam(modelenv, GRB_INT_PAR_MIPFOCUS, focus));
}

void
GurobiBackend::setTimeout(double timeout) {

	GRBenv* modelenv = GRBgetenv(_model);
	GRB_CHECK(GRBsetdblparam(modelenv, GRB_DBL_PAR_TIMELIMIT, timeout));
	LOG_USER(gurobilog) << "using timeout of " << timeout << "s for inference" << std::endl;
}

void
GurobiBackend::setNumThreads(unsigned int numThreads) {

	GRBenv* modelenv = GRBgetenv(_model);
	GRB_CHECK(GRBsetintparam(modelenv, GRB_INT_PAR_THREADS, numThreads));
}

void
GurobiBackend::setVerbose(bool verbose) {

	GRBenv* modelenv = GRBgetenv(_model);

	// setup GRB environment
	if (verbose) {
		GRB_CHECK(GRBsetintparam(modelenv, GRB_INT_PAR_OUTPUTFLAG, 1));
	} else {
		GRB_CHECK(GRBsetintparam(modelenv, GRB_INT_PAR_OUTPUTFLAG, 0));
	}
}

void
GurobiBackend::dumpProblem(std::string filename) {

	// append a random number to avoid overwrites by subsequent calls
	std::stringstream s;
	s << rand() << "_" << filename;

	GRB_CHECK(GRBwrite(_model, s.str().c_str()));

	LOG_USER(gurobilog) << "model dumped to " << s.str() << std::endl;
}

void
GurobiBackend::grbCheck(const char* call, const char* file, int line, int error) {

	if (error)
		UTIL_THROW_EXCEPTION(
				GurobiException,
				"Gurobi error in " << file << ":" << line << ": " << GRBgeterrormsg(_env));
}

#endif // HAVE_GUROBI
