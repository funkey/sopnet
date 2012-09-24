#ifndef IMAGEPROCESSING_MSER_PARAMETERS_COLLECTOR_H__
#define IMAGEPROCESSING_MSER_PARAMETERS_COLLECTOR_H__

#include <pipeline/all.h>
#include "MserParameters.h"

/**
 * A collector that bundles individual paramters into a single MserParameters
 * object.
 */
class ParametersCollector : public pipeline::SimpleProcessNode<> {

public:

	ParametersCollector();

	~ParametersCollector();

private:

	void updateOutputs();

	// inputs
	pipeline::Input<double> _delta;
	pipeline::Input<double> _minArea;
	pipeline::Input<double> _maxArea;
	pipeline::Input<double> _maxVariation;
	pipeline::Input<double> _minDiversity;
	pipeline::Input<bool>   _darkToBright;
	pipeline::Input<bool>   _brightToDark;

	// the graph cut parameters in a single object
	pipeline::Output<MserParameters> _parameters;
};

#endif // IMAGEPROCESSING_MSER_PARAMETERS_COLLECTOR_H__

