#include "MserParametersCollector.h"

ParametersCollector::ParametersCollector() {

	registerInput(_delta, "delta", pipeline::Optional);
	registerInput(_minArea, "min area", pipeline::Optional);
	registerInput(_maxArea, "max area", pipeline::Optional);
	registerInput(_maxVariation, "max variation", pipeline::Optional);
	registerInput(_minDiversity, "min diversity", pipeline::Optional);
	registerInput(_darkToBright, "dark to bright", pipeline::Optional);
	registerInput(_brightToDark, "bright to dark", pipeline::Optional);
	registerOutput(_parameters, "mser parameters");
}

void
ParametersCollector::updateOutputs() {

	if (_delta)
		_parameters->delta        = (int)*_delta;

	if (_minArea)
		_parameters->minArea      = (int)*_minArea;

	if (_maxArea)
		_parameters->maxArea      = (int)*_maxArea;

	if (_maxVariation)
		_parameters->maxVariation = *_maxVariation;

	if (_minDiversity)
		_parameters->minDiversity = *_minDiversity;

	if (_darkToBright)
		_parameters->darkToBright = *_darkToBright;

	if (_brightToDark)
		_parameters->brightToDark = *_brightToDark;
}
