#ifndef INFERENCE_IO_RANDOM_FOREST_HDF5_READER_H__
#define INFERENCE_IO_RANDOM_FOREST_HDF5_READER_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>

class RandomForestHdf5Reader : public pipeline::SimpleProcessNode<> {

public:

	RandomForestHdf5Reader(std::string filename);

private:

	void updateOutputs();

	pipeline::Output<RandomForest> _randomForest;

	std::string _filename;
};

#endif // INFERENCE_IO_RANDOM_FOREST_HDF5_READER_H__

