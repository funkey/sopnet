#ifndef SOPNET_IO_SUBPROBLEMS_READER_H__
#define SOPNET_IO_SUBPROBLEMS_READER_H__

#include <istream>

#include <pipeline/all.h>

#include <sopnet/inference/Subproblems.h>

/**
 * This process node reads problem descriptions from a stream and creates a 
 * Problem for each of them.
 */
class SubproblemsReader : public pipeline::SimpleProcessNode<> {

public:

	SubproblemsReader(const std::string& stream);

	SubproblemsReader(const SubproblemsReader& other);

	~SubproblemsReader();

	SubproblemsReader& operator=(const SubproblemsReader& other);

private:

	void copy(const SubproblemsReader& other);
	void free();

	bool readStdIn() const;

	void updateOutputs();

	void readSubproblem(unsigned int i);
	void readVariable(Problem& problem, unsigned int i);
	void readConstraint(Problem& problem, unsigned int i);

	pipeline::Output<Subproblems> _subproblems;

	std::string   _streamName;
	std::istream* _stream;
	std::filebuf* _fb;
};

#endif // SOPNET_IO_SUBPROBLEMS_READER_H__

