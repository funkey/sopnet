#ifndef INFERENCE_IO_RANDOM_FOREST_HDF5_WRITER_H__
#define INFERENCE_IO_RANDOM_FOREST_HDF5_WRITER_H__

#include <pipeline.h>
#include <inference/RandomForest.h>

class RandomForestHdf5Writer : public pipeline::SimpleProcessNode {

public:

	RandomForestHdf5Writer(std::string filename);

	void write();

private:

	void updateOutputs() {}

	pipeline::Input<RandomForest> _randomForest;

	std::string _filename;
};

#endif // INFERENCE_IO_RANDOM_FOREST_HDF5_WRITER_H__

