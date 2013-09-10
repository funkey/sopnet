#ifndef SOPNET_IO_SOLUTIONS_WRITER_H__
#define SOPNET_IO_SOLUTIONS_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/inference/Solutions.h>
#include <sopnet/inference/Problems.h>

class SolutionsWriter : public pipeline::SimpleProcessNode<> {

public:

	SolutionsWriter(const std::string& stream);

	SolutionsWriter(const SolutionsWriter& other);

	~SolutionsWriter();

	SolutionsWriter& operator=(const SolutionsWriter& other);

	/**
	 * Update the inputs and write the solutions.
	 */
	void write();

private:

	void copy(const SolutionsWriter& other);
	void free();

	bool writeStdOut() const;

	void writeSolution(const Solution& solution, ProblemConfiguration& configuration);

	void updateOutputs() {}

	pipeline::Input<Solutions> _solutions;
	pipeline::Input<Problems>  _problems;

	std::string   _streamName;
	std::ostream* _stream;
	std::filebuf* _fb;
};

#endif // SOPNET_IO_SOLUTIONS_WRITER_H__

