#ifndef SOPNET_EVALUATION_HAMMING_DISTANCE_ERRORS_H__
#define SOPNET_EVALUATION_HAMMING_DISTANCE_ERRORS_H__

#include "Errors.h"

class HammingDistanceErrors : public Errors {

public:

	HammingDistanceErrors() :
		_falsePositives(0),
		_falseNegatives(0) {}

	void setNumFalsePositives(unsigned int falsePositives) { _falsePositives = falsePositives; }
	void setNumFalseNegatives(unsigned int falseNegatives) { _falseNegatives = falseNegatives; }
	unsigned int getNumFalsePositives() { return _falsePositives; }
	unsigned int getNumFalseNegatives() { return _falseNegatives; }
	unsigned int getNumMissclassified() { return _falsePositives + _falseNegatives; }

	std::string errorHeader() { return "Ham_FP\tHam_FN\tHam_SUM"; }

	std::string errorString() {

		std::stringstream ss;
		ss
				<< getNumFalsePositives() << "\t"
				<< getNumFalseNegatives() << "\t"
				<< getNumMissclassified();

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss
				<<   "Ham FP: " << getNumFalsePositives()
				<< ", Ham FN: " << getNumFalseNegatives()
				<< ", Ham Total: " << getNumMissclassified();

		return ss.str();
	}

private:

	unsigned int _falsePositives;
	unsigned int _falseNegatives;
};

#endif // SOPNET_EVALUATION_HAMMING_DISTANCE_ERRORS_H__

