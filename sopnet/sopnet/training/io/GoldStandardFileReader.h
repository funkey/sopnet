#ifndef SOPNET_TRAINING_IO_GOLD_STANDARD_FILE_READER_H__
#define SOPNET_TRAINING_IO_GOLD_STANDARD_FILE_READER_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/Segments.h>

class GoldStandardFileReader : public pipeline::SimpleProcessNode<> {

public:

	GoldStandardFileReader(const std::string& filename);

private:

	void updateOutputs();

	pipeline::Input<Segments>  _allSegments;
	pipeline::Output<Segments> _goldStandard;
	pipeline::Output<Segments> _negativeSamples;

	std::string _filename;
};

#endif // SOPNET_TRAINING_IO_GOLD_STANDARD_FILE_READER_H__

