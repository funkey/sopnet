/**
 * Reads subproblem descriptions from a file or std::cin and writes a solution 
 * to a file or std::cout.
 */

#include <iostream>
#include <string>

#include <sopnet/io/SubproblemsReader.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

util::ProgramOption optionProblemInput(
		util::_long_name        = "in",
		util::_short_name       = "i",
		util::_description_text = "The problem description file or - to read from std::cin.");

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
		pipeline::Process<SubproblemsReader> problemReader(optionProblemInput.as<std::string>());

		/********
		 * TEST *
		 ********/

		LOG_USER(logger::out) << "starting test" << std::endl;

		// ask for some values to trigger update in SubproblemsReader
		pipeline::Value<Subproblems> problems = problemReader->getOutput("problems");
		boost::shared_ptr<Problem> problem = problems->getProblem(0);

	} catch (boost::exception& e) {

		if (boost::get_error_info<error_message>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<error_message>(e);

		LOG_ERROR(logger::out) << std::endl;

		if (boost::get_error_info<stack_trace>(e))
			LOG_ERROR(logger::out) << *boost::get_error_info<stack_trace>(e);
	}
}
