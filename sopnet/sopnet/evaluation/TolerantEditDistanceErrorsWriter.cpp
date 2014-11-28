#include <boost/filesystem.hpp>
#include <pipeline/Process.h>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include "TolerantEditDistanceErrorsWriter.h"

TolerantEditDistanceErrorsWriter::TolerantEditDistanceErrorsWriter() {

	registerInput(_groundTruth, "ground truth");
	registerInput(_reconstruction, "reconstruction");
	registerInput(_tedErrors, "ted errors");
}

void
TolerantEditDistanceErrorsWriter::write(const std::string& baseDirectory) {

	updateInputs();

	boost::filesystem::path splitDirectory(baseDirectory + "/splits");
	boost::filesystem::path mergeDirectory(baseDirectory + "/merges");

	if (!boost::filesystem::exists(baseDirectory))
		boost::filesystem::create_directory(baseDirectory);
	if (!boost::filesystem::exists(splitDirectory))
		boost::filesystem::create_directory(splitDirectory);
	if (!boost::filesystem::exists(mergeDirectory))
		boost::filesystem::create_directory(mergeDirectory);

	std::set<float> splitLabels = _tedErrors->getSplitLabels();
	std::set<float> mergeLabels = _tedErrors->getMergeLabels();

	foreach (float splitLabel, splitLabels) {

		std::set<float> splits = _tedErrors->getSplits(splitLabel);

		std::stringstream dirName;
		dirName << baseDirectory << "/splits/split_" << splitLabel;

		writeStack(dirName.str(), "groundtruth", *_groundTruth, splitLabel);
		writeStack(dirName.str(), "reconstruction", *_reconstruction, splits);
	}

	foreach (float mergeLabel, mergeLabels) {

		std::set<float> merges = _tedErrors->getMerges(mergeLabel);

		std::stringstream dirName;
		dirName << baseDirectory << "/merges/merge_" << mergeLabel;

		writeStack(dirName.str(), "groundtruth", *_groundTruth, merges);
		writeStack(dirName.str(), "reconstruction", *_reconstruction, mergeLabel);
	}
}

void
TolerantEditDistanceErrorsWriter::writeStack(
		const std::string&  dirName,
		const std::string&  stackName,
		ImageStack&         stack,
		float               label) {

	std::set<float> labels;
	labels.insert(label);

	writeStack(dirName, stackName, stack, labels);
}

void
TolerantEditDistanceErrorsWriter::writeStack(
		const std::string&     dirName,
		const std::string&     stackName,
		ImageStack&            stack,
		const std::set<float>& labels) {

	unsigned int width  = stack.width();
	unsigned int height = stack.height();

	// create image stack
	boost::shared_ptr<ImageStack> result = boost::make_shared<ImageStack>();

	foreach (boost::shared_ptr<Image> srcImage, stack) {

		boost::shared_ptr<Image> dstImage = boost::make_shared<Image>(width, height);

		Image::iterator src, dst;

		for (src = srcImage->begin(), dst = dstImage->begin(); src != srcImage->end(); src++, dst++) {

			if (labels.count(*src))
				*dst = *src;
			else
				*dst = 0;
		}

		result->add(dstImage);
	}

	if (!boost::filesystem::exists(dirName))
		boost::filesystem::create_directory(dirName);

	pipeline::Process<ImageStackDirectoryWriter> writer(dirName + "/" + stackName);
	writer->setInput(result);
	writer->write();
}
