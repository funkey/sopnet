/**
 * Reads problem descriptions from a file or std::cin and writes a solution 
 * to a file or std::cout.
 */

#include <iostream>
#include <string>

#include <sopnet/io/ProblemsReader.h>
#include <sopnet/io/SolutionsWriter.h>
#include <sopnet/inference/ProblemsSolver.h>
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

		// create problem reader
		pipeline::Process<ProblemsReader> problemsReader(optionProblemInput.as<std::string>());

		// create problems solver
		pipeline::Process<ProblemsSolver> problemsSolver;

		// create solutions writer
		pipeline::Process<SolutionsWriter> solutionsWriter(optionSolutionOutput.as<std::string>());

		// create pipeline
		problemsSolver->setInput("problems", problemsReader->getOutput());
		solutionsWriter->setInput("solutions", problemsSolver->getOutput("solutions"));
		solutionsWriter->setInput("problems", problemsReader->getOutput());

		// write solution
		solutionsWriter->write();

	} catch (boost::exception& e) {

		if (boost::get_error_info<error_message>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<error_message>(e);

		LOG_ERROR(logger::out) << std::endl;

		if (boost::get_error_info<stack_trace>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<stack_trace>(e);
	}
}
