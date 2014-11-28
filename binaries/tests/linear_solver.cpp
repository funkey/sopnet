#include <iostream>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <inference/LinearSolver.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>

int main(int argc, char** argv) {

	try {

		// init command line parser
		util::ProgramOptions::init(argc, argv);

		// init logger
		logger::LogManager::init();
		

		const unsigned int numVariables = 100;

		// create a linear solver
		pipeline::Process<LinearSolver> solver;

		// create linear constraints
		boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>();

		// create objective
		boost::shared_ptr<LinearObjective> objective = boost::make_shared<LinearObjective>(numVariables);
		for (unsigned int i = 0; i < numVariables; i++)
			objective->setCoefficient(i, 1.0); // switching on is expensive

		// setup solver
		solver->setInput("objective", objective);
		solver->setInput("linear constraints", constraints);
		solver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

		// create a solution
		pipeline::Value<Solution> solution = solver->getOutput();

		// pin each variable
		for (unsigned int i = 0; i < numVariables; i++) {

			std::cout << "pinning variable " << i << std::endl;

			solver->pinVariable(i, 1);

			for (unsigned int j = 0; j < numVariables; j++) {

				// was the value pinned?
				if (j == i) {

					if ((*solution)[j] != 1)
						std::cerr << "variable " << j << " has wrong value: " << (*solution)[j] << std::endl;

				} else {

					if ((*solution)[j] != 0)
						std::cerr << "variable " << j << " has wrong value: " << (*solution)[j] << std::endl;
				}
			}

			solver->unpinVariable(i);
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
