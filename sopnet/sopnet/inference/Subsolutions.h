#ifndef SOPNET_INFERENCE_SUBSOLUTIONS_H__
#define SOPNET_INFERENCE_SUBSOLUTIONS_H__

#include <pipeline/all.h>
#include <inference/Solution.h>

/**
 * Collection of solutions for a set of subproblems.
 */
class Subsolutions : public pipeline::Data {

public:

	void addSolution(boost::shared_ptr<Solution> solution) {

		_solutions.push_back(solution);
	}

	boost::shared_ptr<Solution> getSolution(unsigned int i) {

		return _solutions[i];
	}

	unsigned int size() {

		return _solutions.size();
	}

	void clear() {

		_solutions.clear();
	}

private:

	std::vector<boost::shared_ptr<Solution> > _solutions;
};

#endif // SOPNET_INFERENCE_SUBSOLUTIONS_H__

