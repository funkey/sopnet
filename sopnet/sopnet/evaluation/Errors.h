#ifndef SOPNET_EVALUATION_ERRORS_H__
#define SOPNET_EVALUATION_ERRORS_H__

#include <string>
#include <pipeline/Data.h>

/**
 * Base class for error data structures.
 */
class Errors : public pipeline::Data {

public:

	/**
	 * Return a string describing the return value of errorString(), e.g.,
	 *
	 *    "FP FN SUM"
	 *
	 * to say that errorString() returns a string with three numbers that are 
	 * called FP, FN, and SUM, respectively.
	 */
	virtual std::string errorHeader() = 0;

	/**
	 * Return a string representation of the errors that can readily be stored 
	 * in a plot file, e.g.,
	 *
	 *    "12 32 44"
	 */
	virtual std::string errorString() = 0;

	/**
	 * Return a human readable string of the errors, e.g.,
	 *
	 *    "False Positives: 12, False Negatives: 32, Total: 44"
	 */
	virtual std::string humanReadableErrorString() = 0;
};

#endif // SOPNET_EVALUATION_ERRORS_H__

