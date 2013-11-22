#ifndef CELLTRACKER_TRACKLET_COST_FUNCTION_PARAMETERS_H__
#define CELLTRACKER_TRACKLET_COST_FUNCTION_PARAMETERS_H__

#include <vector>

#include <pipeline/Data.h>

class LinearCostFunctionParameters : public pipeline::Data {

public:

	void setWeights(const std::vector<double>& weights) { _weights = weights; }

	const std::vector<double>& getWeights() { return _weights; }

private:

	std::vector<double> _weights;
};

#endif // CELLTRACKER_TRACKLET_COST_FUNCTION_PARAMETERS_H__

