#include "GridSearch.h"

util::ProgramOption optionPriorGridSearchStartEnd(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStartEnd",
		util::_description_text = "Start value of the grid search for priors on the end segments.",
		util::_default_value    = "0");

util::ProgramOption optionPriorGridSearchEndEnd(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchEndEnd",
		util::_description_text = "End value of the grid search for priors on the end segments.",
		util::_default_value    = "10000000");

util::ProgramOption optionPriorGridSearchStepEnd(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStepEnd",
		util::_description_text = "Incement of the grid search for priors on the end segments.",
		util::_default_value    = "1000000");

util::ProgramOption optionPriorGridSearchStartContinuation(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStartContinuation",
		util::_description_text = "Start value of the grid search for priors on the continuation segments.",
		util::_default_value    = "0");

util::ProgramOption optionPriorGridSearchEndContinuation(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchEndContinuation",
		util::_description_text = "Continuation value of the grid search for priors on the continuation segments.",
		util::_default_value    = "10000000");

util::ProgramOption optionPriorGridSearchStepContinuation(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStepContinuation",
		util::_description_text = "Incement of the grid search for priors on the continuation segments.",
		util::_default_value    = "1000000");

util::ProgramOption optionPriorGridSearchStartBranch(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStartBranch",
		util::_description_text = "Start value of the grid search for priors on the branch segments.",
		util::_default_value    = "0");

util::ProgramOption optionPriorGridSearchEndBranch(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchEndBranch",
		util::_description_text = "Branch value of the grid search for priors on the branch segments.",
		util::_default_value    = "10000000");

util::ProgramOption optionPriorGridSearchStepBranch(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorGridSearchStepBranch",
		util::_description_text = "Incement of the grid search for priors on the branch segments.",
		util::_default_value    = "1000000");

GridSearch::GridSearch() :
		_priorCostFunctionParameters(boost::make_shared<PriorCostFunctionParameters>()) {

	// set initial values
	_priorCostFunctionParameters->priorEnd          = optionPriorGridSearchStartEnd;
	_priorCostFunctionParameters->priorContinuation = optionPriorGridSearchStartContinuation;
	_priorCostFunctionParameters->priorBranch       = optionPriorGridSearchStartBranch;

	registerOutput(_priorCostFunctionParameters, "prior cost parameters");
}


bool
GridSearch::next() {

	_priorCostFunctionParameters->priorEnd += optionPriorGridSearchStepEnd.as<double>();

	if (_priorCostFunctionParameters->priorEnd > optionPriorGridSearchEndEnd.as<double>()) {

		_priorCostFunctionParameters->priorEnd = optionPriorGridSearchStartEnd.as<double>();

		_priorCostFunctionParameters->priorContinuation += optionPriorGridSearchStepContinuation.as<double>();

		if (_priorCostFunctionParameters->priorContinuation > optionPriorGridSearchEndContinuation.as<double>()) {

			_priorCostFunctionParameters->priorContinuation = optionPriorGridSearchStartContinuation.as<double>();

			_priorCostFunctionParameters->priorBranch += optionPriorGridSearchStepBranch.as<double>();
		}
	}

	if (
			_priorCostFunctionParameters->priorEnd          > optionPriorGridSearchEndEnd.as<double>()          ||
			_priorCostFunctionParameters->priorContinuation > optionPriorGridSearchEndContinuation.as<double>() ||
			_priorCostFunctionParameters->priorBranch       > optionPriorGridSearchEndBranch.as<double>()) {

		return false;
	}

	setDirty(_priorCostFunctionParameters);

	return true;
}

