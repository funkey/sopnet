#ifndef INFERENCE_SOLUTION_H__
#define INFERENCE_SOLUTION_H__

#include <pipeline.h>

class Solution : public pipeline::Data {

public:

	Solution(unsigned int size = 0);

	void resize(unsigned int size);

	unsigned int size() { return _solution.size(); }

	double& operator[](unsigned int i) { return _solution[i]; }

	std::vector<double>& getVector() { return _solution; }

private:

	std::vector<double> _solution;
};

#endif // INFERENCE_SOLUTION_H__

