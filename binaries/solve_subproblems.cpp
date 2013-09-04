/**
 * Reads subproblem descriptions from a file or std::cin and writes a solution 
 * to a file or std::cout.
 */

#include <iostream>
#include <string>

#include <sopnet/io/SubproblemsReader.h>
#include <sopnet/io/SubsolutionsWriter.h>
#include <sopnet/inference/SubproblemsSolver.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

util::ProgramOption optionProblemInput(
		util::_long_name        = "in",
		util::_short_name       = "i",
		util::_description_text = "The problem description file or - to read from std::cin.",
		util::_default_value    = "-");

util::ProgramOption optionSolutionOutput(
		util::_long_name        = "out",
		util::_short_name       = "o",
		util::_description_text = "The problem description file or - to read from std::cin.",
		util::_default_value    = "-");

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		logger::LogManager::init();

		// init signal handler
		util::SignalHandler::init();

		// create subproblem reader
		pipeline::Process<SubproblemsReader> subproblemsReader(optionProblemInput.as<std::string>());

		// create subproblems solver
		pipeline::Process<SubproblemsSolver> subproblemsSolver;

		// create subsolutions writer
		pipeline::Process<SubsolutionsWriter> subsolutionsWriter(optionSolutionOutput.as<std::string>());

		// create pipeline
		subproblemsSolver->setInput("subproblems", subproblemsReader->getOutput());
		subsolutionsWriter->setInput("subsolutions", subproblemsSolver->getOutput("subsolutions"));
		subsolutionsWriter->setInput("subproblems", subproblemsReader->getOutput());

		// write solution
		subsolutionsWriter->write();

	} catch (boost::exception& e) {

		if (boost::get_error_info<error_message>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<error_message>(e);

		LOG_ERROR(logger::out) << std::endl;

		if (boost::get_error_info<stack_trace>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<stack_trace>(e);
	}
}
