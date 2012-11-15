#include "Errors.h"

std::ostream& operator<<(std::ostream& os, const Errors& errors) {

	os
			<< "FP: " << errors.numFalsePositives << ", "
			<< "FN: " << errors.numFalseNegatives << ", "
			<< "FS: " << errors.numFalseSplits << ", "
			<< "FM: " << errors.numFalseMerges;

	return os;
}
