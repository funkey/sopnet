#ifndef INFERENCE_LINEAR_OBJECTIVE_H__
#define INFERENCE_LINEAR_OBJECTIVE_H__

#include "QuadraticObjective.h"

class LinearObjective : public QuadraticObjective {

public:

	LinearObjective(unsigned int size = 0) : QuadraticObjective(size) {}

private:

	using QuadraticObjective::setQuadraticCoefficient;
};

#endif // INFERENCE_OBJECTIVE_H__

