#ifndef SOPNET_EVALUATION_ERRORS_H__
#define SOPNET_EVALUATION_ERRORS_H__

#include <pipeline/all.h>

struct Errors : public pipeline::Data {

	Errors() :
		numFalsePositives(0),
		numFalseNegatives(0),
		numFalseSplits(0),
		numFalseMerges(0) {}

	int numFalsePositives;
	int numFalseNegatives;
	int numFalseSplits;
	int numFalseMerges;

	int total() {

		return
				numFalsePositives +
				numFalseNegatives +
				numFalseSplits +
				numFalseMerges;
	}

	Errors operator+(const Errors& other) const {

		Errors result;

		result.numFalsePositives = numFalsePositives + other.numFalsePositives;
		result.numFalseNegatives = numFalseNegatives + other.numFalseNegatives;
		result.numFalseSplits    = numFalseSplits + other.numFalseSplits;
		result.numFalseMerges    = numFalseMerges + other.numFalseMerges;

		return result;
	}
};

std::ostream& operator<<(std::ostream& os, const Errors& errors);

#endif // SOPNET_EVALUATION_ERRORS_H__

