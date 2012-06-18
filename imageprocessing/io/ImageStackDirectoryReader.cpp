#include <imageprocessing/Image.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/io/ImageReader.h>
#include "ImageStackDirectoryReader.h"

logger::LogChannel imagestackdirectoryreaderlog("imagestackdirectoryreaderlog", "[ImageStackDirectoryReader] ");

ImageStackDirectoryReader::ImageStackDirectoryReader(const std::string& directory) :
	_stackAssembler(boost::make_shared<StackAssembler>()),
	_directory(directory) {

	LOG_DEBUG(imagestackdirectoryreaderlog) << "reading from directory " << _directory << std::endl;

	boost::filesystem::path dir(_directory);

	if (!boost::filesystem::exists(dir))
		BOOST_THROW_EXCEPTION(IOError() << error_message(_directory + " does not exist"));

	if (!boost::filesystem::is_directory(dir))
		BOOST_THROW_EXCEPTION(IOError() << error_message(_directory + " is not a directory"));

	// get a sorted list of image files
	std::vector<boost::filesystem::path> sorted;
	std::copy(
			boost::filesystem::directory_iterator(dir),
			boost::filesystem::directory_iterator(),
			back_inserter(sorted));
	std::sort(sorted.begin(), sorted.end());

	LOG_DEBUG(imagestackdirectoryreaderlog) << "directory contains " << sorted.size() << " entries" << std::endl;

	// for every image file in the given directory
	foreach (boost::filesystem::path file, sorted) {

		if (boost::filesystem::is_regular_file(file)) {

			LOG_DEBUG(imagestackdirectoryreaderlog) << "creating reader for " << file << std::endl;

			// add an input to the stack assembler
			boost::shared_ptr<ImageReader> reader = boost::make_shared<ImageReader>(file.string());

			_stackAssembler->addInput(reader->getOutput());
		}
	}

	// expose the result of the stack assembler
	registerOutput(_stackAssembler->getOutput(), "stack");
}

ImageStackDirectoryReader::StackAssembler::StackAssembler() {

	registerInputs(_images, "images");
	registerOutput(_stack, "stack");
}

void
ImageStackDirectoryReader::StackAssembler::updateOutputs() {

	_stack->clear();

	foreach (boost::shared_ptr<Image> image, _images)
		_stack->add(image);
}
