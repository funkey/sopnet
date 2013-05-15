#ifndef SOPNET_IO_STREAM_PROBLEM_READER_H__
#define SOPNET_IO_STREAM_PROBLEM_READER_H__

#include <istream>

#include <pipeline/all.h>

#include <sopnet/inference/Problems.h>

/**
 * This process node reads problem descriptions from a stream and creates a 
 * Problem for each of them.
 */
class StreamProblemReader : public pipeline::SimpleProcessNode<> {

public:

	StreamProblemReader(const std::string& stream);

	StreamProblemReader(const StreamProblemReader& other);

	~StreamProblemReader();

	StreamProblemReader& operator=(const StreamProblemReader& other);

private:

	void copy(const StreamProblemReader& other);
	void free();

	bool readStdIn() const;

	void updateOutputs();

	void readVariable(unsigned int i);
	void readOneConstraint(unsigned int i);
	void readEqualConstraint(unsigned int i);

	pipeline::Output<Problems> _problems;

	std::string   _streamName;
	std::istream* _stream;
	std::filebuf* _fb;
};

#endif // SOPNET_IO_STREAM_PROBLEM_READER_H__

