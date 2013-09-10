#ifndef SOPNET_IO_SOLUTION_READER_H__
#define SOPNET_IO_SOLUTION_READER_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/inference/Solution.h>

class SolutionReader : public pipeline::SimpleProcessNode<> {

public:

	SolutionReader(const std::string& filename) {

		registerOutput(_solution, "solution");
	}

private:

	void updateOutputs() {}

	pipeline::Output<Solution> _solution;
};

#endif // SOPNET_IO_SOLUTION_READER_H__

