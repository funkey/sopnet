#include "SkeletonToleranceFunction.h"

void
SkeletonToleranceFunction::findRelabelCandidates(const std::vector<float>& maxBoundaryDistances) {


	_relabelCandidates.clear();
	for (unsigned int cellIndex = 0; cellIndex < maxBoundaryDistances.size(); cellIndex++) {

		if (isSkeletonCell(cellIndex)) {

			// add all skeleton cells to the relabel candidates
			_relabelCandidates.push_back(cellIndex);

		} else {

			cell_t& cell = (*_cells)[cellIndex];

			// all non-skeleton cells can (and should) be relabeled to 
			// background
			cell.addAlternativeLabel(_backgroundLabel);
			registerPossibleMatch(cell.getGroundTruthLabel(), _backgroundLabel);
		}
	}
}

bool
SkeletonToleranceFunction::isSkeletonCell(unsigned int cellIndex) {

	// a cell is a skeleton cell, if its ground truth label is not the 
	// background
	return (*_cells)[cellIndex].getGroundTruthLabel() != _backgroundLabel;
}
