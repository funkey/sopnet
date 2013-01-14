#include <sstream>

#include <boost/filesystem.hpp>

#include <vigra/impex.hxx>

#include "NeuronsImageWriter.h"

NeuronsImageWriter::NeuronsImageWriter(std::string directory, std::string basename) :
		_directory(directory),
		_basename(basename) {

	registerInput(_idMap, "id map");
}

void
NeuronsImageWriter::write() {

	unsigned int numSections = _idMap->size();

	// prepare the output directory
	boost::filesystem::path directory(_directory);

	if (!boost::filesystem::exists(directory)) {

		boost::filesystem::create_directory(directory);

	} else if (!boost::filesystem::is_directory(directory)) {

		BOOST_THROW_EXCEPTION(IOError() << error_message(std::string("\"") + _directory + "\" is not a directory") << STACK_TRACE);
	}

	// make sure we have a recent id map
	updateInputs();

	// save output images
	for (unsigned int i = 0; i < numSections; i++) {

		std::stringstream filename;

		filename << _directory << "/" << _basename << std::setw(4) << std::setfill('0') << i << ".tiff";

		vigra::exportImage(srcImageRange(*(*_idMap)[i]), vigra::ImageExportInfo(filename.str().c_str()));
	}
}
