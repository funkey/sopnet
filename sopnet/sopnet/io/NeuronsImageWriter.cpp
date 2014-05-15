#include <sstream>

#include <boost/filesystem.hpp>

#include <vigra/impex.hxx>

#include "NeuronsImageWriter.h"

NeuronsImageWriter::NeuronsImageWriter(
		std::string  directory,
		std::string  basename,
		unsigned int firstSection) :
		_directory(directory),
		_basename(basename),
		_firstSection(firstSection) {

	registerInput(_idMap, "id map");
	registerInput(_annotation, "annotation", pipeline::Optional);
}

void
NeuronsImageWriter::write() {

	// make sure we have a recent id map
	updateInputs();

	unsigned int numSections = _idMap->size();

	if (_annotation.isSet()) {

		_directory += std::string("_") + boost::lexical_cast<std::string>(*_annotation);
	}

	// prepare the output directory
	boost::filesystem::path directory(_directory);

	if (!boost::filesystem::exists(directory)) {

		boost::filesystem::create_directory(directory);

	} else if (!boost::filesystem::is_directory(directory)) {

		BOOST_THROW_EXCEPTION(IOError() << error_message(std::string("\"") + _directory + "\" is not a directory") << STACK_TRACE);
	}

	// save output images
	for (unsigned int i = 0; i < numSections; i++) {

		std::stringstream filename;

		filename << _directory << "/" << _basename << std::setw(4) << std::setfill('0') << (i + _firstSection) << ".tiff";

		vigra::exportImage(srcImageRange(*(*_idMap)[i]), vigra::ImageExportInfo(filename.str().c_str()));
	}
}
