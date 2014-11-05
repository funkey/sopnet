#ifndef SOPNET_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__
#define SOPNET_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__

#include "Errors.h"
#include <sstream>

class VariationOfInformationErrors : public Errors {

public:

	VariationOfInformationErrors() :
			_splitEntropy(0),
			_mergeEntropy(0) {}

	/**
	 * Set the conditional entropy H(A|B), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	void setSplitEntropy(double splitEntropy) { _splitEntropy = splitEntropy; }

	/**
	 * Set the conditional entropy H(B|A), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	void setMergeEntropy(double mergeEntropy) { _mergeEntropy = mergeEntropy; }

	/**
	 * Get the conditional entropy H(A|B), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	double getSplitEntropy() { return _splitEntropy; }

	/**
	 * Get the conditional entropy H(B|A), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	double getMergeEntropy() { return _mergeEntropy; }

	/**
	 * Get the total entropy, i.e., the variation of information.
	 */
	double getEntropy() { return _splitEntropy + _mergeEntropy; }

	std::string errorHeader() { return "VOI_SPLIT\tVOI_MERGE\tVOI"; }

	std::string errorString() {

		std::stringstream ss;
		ss << getSplitEntropy() << "\t" << getMergeEntropy() << "\t" << getEntropy();

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss
				<< "VOI split: "   << getSplitEntropy()
				<< ", VOI merge: " << getMergeEntropy()
				<< ", VOI total: " << getEntropy();

		return ss.str();
	}

private:

	double _splitEntropy;
	double _mergeEntropy;
};

#endif // SOPNET_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__

