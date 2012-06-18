#ifndef LINEAR_PROGRAM_SOLVER_FACTORY_H__
#define LINEAR_PROGRAM_SOLVER_FACTORY_H__

// forward declaration
class LinearSolverBackend;

class LinearSolverBackendFactory {

public:

	virtual LinearSolverBackend* createLinearSolverBackend() const = 0;
};

#endif // LINEAR_PROGRAM_SOLVER_FACTORY_H__

