#ifndef SOPNET_IO_PROBLEMS_READER_H__
#define SOPNET_IO_PROBLEMS_READER_H__

#include <istream>

#include <pipeline/all.h>

#include <sopnet/inference/Problems.h>

/**
 * This process node reads problem descriptions from a stream and creates a 
 * Problem for each of them.
 *
 * TODO: rename to ProblemsReader
 */
class ProblemsReader : public pipeline::SimpleProcessNode<> {

public:

	ProblemsReader(const std::string& stream);

	ProblemsReader(const ProblemsReader& other);

	~ProblemsReader();

	ProblemsReader& operator=(const ProblemsReader& other);

private:

	void copy(const ProblemsReader& other);
	void free();

	bool readStdIn() const;

	void updateOutputs();

	void readProblem(unsigned int i);
	void readVariable(Problem& problem, unsigned int i);
	void readConstraint(Problem& problem, unsigned int i);

	pipeline::Output<Problems> _problems;

	std::string   _streamName;
	std::istream* _stream;
	std::filebuf* _fb;
};

#endif // SOPNET_IO_PROBLEMS_READER_H__

