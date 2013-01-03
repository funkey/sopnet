#include <sstream>

#include <boost/filesystem.hpp>

#include <vigra/impex.hxx>

#include "NeuronsImageWriter.h"

NeuronsImageWriter::NeuronsImageWriter(std::string directory, std::string basename) :
		_directory(directory),
		_basename(basename) {

	registerInput(_neurons, "neurons");
	registerInput(_reference, "reference image stack");
}

void
NeuronsImageWriter::write() {

	// prepare the output directory
	boost::filesystem::path directory(_directory);

	if (!boost::filesystem::exists(directory)) {

		boost::filesystem::create_directory(directory);

	} else if (!boost::filesystem::is_directory(directory)) {

		BOOST_THROW_EXCEPTION(IOError() << error_message(std::string("\"") + _directory + "\" is not a directory") << STACK_TRACE);
	}

	// make sure we have a recent _neurons
	updateInputs();

	// get widht and height of output images from reference image stack
	unsigned int numSections = _reference->size();
	unsigned int width  = _reference->width();
	unsigned int height = _reference->height();

	// prepare output images
	std::vector<id_image> idImages(numSections, id_image(id_image::difference_type(width, height), (unsigned int)0));

	// draw each neuron to output images
	unsigned int id = 1;
	foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

		foreach (boost::shared_ptr<EndSegment> end, neuron->getEnds()) {

			drawSlice(*end->getSlice(), idImages, id);
		}

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron->getContinuations()) {

			if (continuation->getDirection() == Left)

				drawSlice(*continuation->getSourceSlice(), idImages, id);

			else

				drawSlice(*continuation->getTargetSlice(), idImages, id);
		}

		foreach (boost::shared_ptr<BranchSegment> branch, neuron->getBranches()) {

			if (branch->getDirection() == Left)

				drawSlice(*branch->getSourceSlice(), idImages, id);

			else {

				drawSlice(*branch->getTargetSlice1(), idImages, id);
				drawSlice(*branch->getTargetSlice2(), idImages, id);
			}
		}

		id++;
	}

	// save output images
	for (unsigned int i = 0; i < numSections; i++) {

		std::stringstream filename;

		filename << _directory << "/" << _basename << std::setw(4) << std::setfill('0') << i << ".tiff";

		vigra::exportImage(srcImageRange(idImages[i]), vigra::ImageExportInfo(filename.str().c_str()));
	}
}

void
NeuronsImageWriter::drawSlice(const Slice& slice, std::vector<id_image>& images, unsigned int id) {

	unsigned int section = slice.getSection();

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels())
		images[section](pixel.x, pixel.y) = id;
}
