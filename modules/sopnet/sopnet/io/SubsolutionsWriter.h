#ifndef SOPNET_IO_SUBSOLUTIONS_WRITER_H__
#define SOPNET_IO_SUBSOLUTIONS_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/inference/Subsolutions.h>
#include <sopnet/inference/Subproblems.h>

class SubsolutionsWriter : public pipeline::SimpleProcessNode<> {

public:

	SubsolutionsWriter(const std::string& stream);

	SubsolutionsWriter(const SubsolutionsWriter& other);

	~SubsolutionsWriter();

	SubsolutionsWriter& operator=(const SubsolutionsWriter& other);

	/**
	 * Update the inputs and write the subsolutions.
	 */
	void write();

private:

	void copy(const SubsolutionsWriter& other);
	void free();

	bool writeStdOut() const;

	void writeSubsolution(const Solution& solution, ProblemConfiguration& configuration);

	void updateOutputs() {}

	pipeline::Input<Subsolutions> _subsolutions;
	pipeline::Input<Subproblems>  _subproblems;

	std::string   _streamName;
	std::ostream* _stream;
	std::filebuf* _fb;
};

#endif // SOPNET_IO_SUBSOLUTIONS_WRITER_H__

