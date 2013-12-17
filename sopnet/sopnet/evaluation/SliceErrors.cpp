#include "SliceErrors.h"

std::ostream& operator<<(std::ostream& os, const SliceErrors& sliceErrors) {

	os
			<< "FP: " << sliceErrors.numFalsePositives() << ", "
			<< "FN: " << sliceErrors.numFalseNegatives() << ", "
			<< "FS: " << sliceErrors.numFalseSplits() << ", "
			<< "FM: " << sliceErrors.numFalseMerges();

	return os;
}
