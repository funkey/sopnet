#ifndef SOPNET_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__
#define SOPNET_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__

#include "DistanceToleranceFunction.h"

/**
 * Specialization of a distance tolerance function that operates on skeleton 
 * ground truth. The distance tolerance criterion specifies how far away a 
 * skeleton is allowed to be from the reconstruction without causing an error.
 */
class SkeletonToleranceFunction : public DistanceToleranceFunction {

private:

	// for the skeleton criterion, each cell is allowed to be relabeled
	virtual void findRelabelCandidates(const std::vector<float>& maxBoundaryDistances);

	bool isSkeletonCell(unsigned int cellIndex);
};

#endif // SOPNET_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__

