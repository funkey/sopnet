#include "RandomForestHdf5Reader.h"

RandomForestHdf5Reader::RandomForestHdf5Reader(std::string filename) :
	_filename(filename) {

	registerOutput(_randomForest, "random forest");
}

void
RandomForestHdf5Reader::updateOutputs() {

	_randomForest->read(_filename);
}
