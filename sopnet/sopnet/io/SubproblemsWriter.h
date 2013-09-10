#ifndef SOPNET_IO_SUBPROBLEMS_WRITER_H__
#define SOPNET_IO_SUBPROBLEMS_WRITER_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/inference/Subproblems.h>

class SubproblemsWriter : public pipeline::SimpleProcessNode<> {

public:

	SubproblemsWriter(const std::string& filename);

	void write(std::string filename = "");

private:

	void updateOutputs() {}

	pipeline::Input<Subproblems> _subproblems;

	std::string _filename;
};

#endif // SOPNET_IO_SUBPROBLEMS_WRITER_H__

