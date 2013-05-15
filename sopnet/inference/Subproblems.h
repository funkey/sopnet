#ifndef SOPNET_INFERENCE_SUBPROBLEMS_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_H__

#include <pipeline/all.h>

#include "Problem.h"

/**
 * A collection of problems that should be solved jointly.
 */
class Subproblems : public pipeline::Data {

public:

	void addProblem(boost::shared_ptr<Problem> problem) {

		_problems.push_back(problem);
	}

	boost::shared_ptr<Problem> getProblem(unsigned int i) {

		return _problems[i];
	}

	unsigned int size() {

		return _problems.size();
	}

	void clear() {

		_problems.clear();
	}

private:

	std::vector<boost::shared_ptr<Problem> > _problems;
};

#endif // SOPNET_INFERENCE_SUBPROBLEMS_H__

