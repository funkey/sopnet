#ifndef SOPNET_INFERENCE_PROBLEMS_H__
#define SOPNET_INFERENCE_PROBLEMS_H__

#include <pipeline/all.h>

#include "Problem.h"

/**
 * A collection of problems.
 */
class Problems : public pipeline::Data {

	typedef std::vector<boost::shared_ptr<Problem> > problems_type;
	typedef problems_type::iterator                  iterator;
	typedef problems_type::const_iterator            const_iterator;

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

	problems_type::iterator end() {

		return _problems.end();
	}

	problems_type::iterator begin() {

		return _problems.begin();
	}

	problems_type::const_iterator end() const {

		return _problems.end();
	}

	problems_type::const_iterator begin() const {

		return _problems.begin();
	}

private:

	problems_type _problems;
};

#endif // SOPNET_INFERENCE_PROBLEMS_H__

