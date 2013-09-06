#ifndef INFERENCE_SOLUTION_H__
#define INFERENCE_SOLUTION_H__

#include <pipeline/all.h>

class Solution : public pipeline::Data {

public:

	Solution(unsigned int size = 0);

	void resize(unsigned int size);

	unsigned int size() const { return _solution.size(); }

	const double& operator[](unsigned int i) const { return _solution[i]; }

	double& operator[](unsigned int i) { return _solution[i]; }

	std::vector<double>& getVector() { return _solution; }

private:

	std::vector<double> _solution;
};

#endif // INFERENCE_SOLUTION_H__

