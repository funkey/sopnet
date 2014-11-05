#ifndef SOPNET_EVALUATION_ANISOTROPIC_EDIT_DISTANCE_ERRORS_H__
#define SOPNET_EVALUATION_ANISOTROPIC_EDIT_DISTANCE_ERRORS_H__

#include <pipeline/all.h>
#include "Errors.h"

class AnisotropicEditDistanceErrors : public Errors {

public:

	unsigned int numFalsePositives() const { return _falsePositives.size(); }
	unsigned int numFalseNegatives() const { return _falseNegatives.size(); }
	unsigned int numFalseSplits()    const { return _falseSplits.size(); }
	unsigned int numFalseMerges()    const { return _falseMerges.size(); }

	int total() {

		return
				numFalsePositives() +
				numFalseNegatives() +
				numFalseSplits() +
				numFalseMerges();
	}

	std::set<int>& falsePositives() { return _falsePositives; }
	std::set<int>& falseNegatives() { return _falseNegatives; }

	std::set<std::pair<int, int> >& falseSplits() { return _falseSplits; }
	std::set<std::pair<int, int> >& falseMerges() { return _falseMerges; }

	const std::set<int>& falsePositives() const { return _falsePositives; }
	const std::set<int>& falseNegatives()  const{ return _falseNegatives; }

	const std::set<std::pair<int, int> >& falseSplits() const { return _falseSplits; }
	const std::set<std::pair<int, int> >& falseMerges() const { return _falseMerges; }

	AnisotropicEditDistanceErrors operator+(const AnisotropicEditDistanceErrors& other) const {

		AnisotropicEditDistanceErrors result;

		result.falsePositives() = falsePositives();
		result.falseNegatives() = falseNegatives();
		result.falseSplits()    = falseSplits();
		result.falseMerges()    = falseMerges();
		result.falsePositives().insert(other.falsePositives().begin(), other.falsePositives().end());
		result.falseNegatives().insert(other.falseNegatives().begin(), other.falseNegatives().end());
		result.falseSplits().insert(other.falseSplits().begin(), other.falseSplits().end());
		result.falseMerges().insert(other.falseMerges().begin(), other.falseMerges().end());

		return result;
	}

	std::string errorHeader() { return "AED_FP\tAED_FN\tAED_FS\tAED_FM\tAED_SUM"; }

	std::string errorString() {

		std::stringstream ss;
		ss
				<< numFalsePositives() << "\t"
				<< numFalseNegatives() << "\t"
				<< numFalseSplits() << "\t"
				<< numFalseMerges() << "\t"
				<< total();

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss
				<<   "AED FP: " << numFalsePositives()
				<< ", AED FN: " << numFalseNegatives()
				<< ", AED FS: " << numFalseSplits()
				<< ", AED FM: " << numFalseMerges()
				<< ", AED Total: " << total();

		return ss.str();
	}

private:

	// the slice ids of the false positives and negatives
	std::set<int> _falsePositives;
	std::set<int> _falseNegatives;

	// pairs of slice ids of the false splits and merges
	std::set<std::pair<int, int> > _falseSplits;
	std::set<std::pair<int, int> > _falseMerges;
};

std::ostream& operator<<(std::ostream& os, const AnisotropicEditDistanceErrors& sliceErrors);

#endif // SOPNET_EVALUATION_ANISOTROPIC_EDIT_DISTANCE_ERRORS_H__

