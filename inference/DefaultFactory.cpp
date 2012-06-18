#include "DefaultFactory.h"

#ifdef HAVE_GUROBI
#include "GurobiBackend.h"
#endif

#ifdef HAVE_CPLEX
#include "CplexBackend.h"
#endif

LinearSolverBackend*
DefaultFactory::createLinearSolverBackend() const {

// by default, create a gurobi backend
#ifdef HAVE_GUROBI

	return new GurobiBackend();

#endif

// if this is not available, create a CPLEX backend
#ifdef HAVE_CPLEX

	return new CplexBackend();

#endif

// if this is not available as well, throw an exception

	BOOST_THROW_EXCEPTION(NoSolverException() << error_message("No linear solver available."));
}

QuadraticSolverBackend*
DefaultFactory::createQuadraticSolverBackend() const {

// by default, create a gurobi backend
#ifdef HAVE_GUROBI

	return new GurobiBackend();

#endif

// if this is not available, create a CPLEX backend
#ifdef HAVE_CPLEX

	return new CplexBackend();

#endif

// if this is not available as well, throw an exception

	BOOST_THROW_EXCEPTION(NoSolverException() << error_message("No linear solver available."));
}
