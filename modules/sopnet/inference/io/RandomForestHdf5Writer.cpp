#include "RandomForestHdf5Writer.h"

RandomForestHdf5Writer::RandomForestHdf5Writer(std::string filename) :
	_filename(filename) {

	registerInput(_randomForest, "random forest");
}

void
RandomForestHdf5Writer::write() {

	updateInputs();

	_randomForest->write(_filename);
}

