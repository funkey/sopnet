#include <fstream>

#include <boost/lexical_cast.hpp>

#include <vigra/impex.hxx>

#include "ProblemGraphWriter.h"

static logger::LogChannel problemgraphwriterlog("problemgraphwriterlog", "[ProblemGraphWriter] ");

util::ProgramOption optionProblemGraphFile(
		util::_module           = "sopnet",
		util::_long_name        = "problemGraphFile",
		util::_description_text = "Path to the problem graph file to produce.",
		util::_default_value    = "problem.graph");

ProblemGraphWriter::ProblemGraphWriter() {

	registerInput(_segments, "segments");
	registerInput(_segmentIdsToVariables, "segment ids map");
	registerInput(_objective, "objective");
	registerInputs(_linearConstraints, "linear constraints");
}

void
ProblemGraphWriter::write(
		const std::string& slicesFile,
		const std::string& segmentsFile,
		const std::string& constraintsFile,
		const std::string& sliceImageDirectory) {

	if (!_segments || !_segmentIdsToVariables || !_objective) {

		LOG_DEBUG(problemgraphwriterlog) << "not all required inputs are present -- skip dumping" << std::endl;
		return;
	}

	updateInputs();

	LOG_DEBUG(problemgraphwriterlog) << "dumping problem graph..." << std::endl;

	writeSlices(slicesFile, sliceImageDirectory);

	writeSegments(segmentsFile);

	writeConstraints();

	LOG_DEBUG(problemgraphwriterlog) << "done" << std::endl;
}

void
ProblemGraphWriter::writeSlices(
		const std::string& slicesFile,
		const std::string& sliceImageDirectory) {

	LOG_DEBUG(problemgraphwriterlog) << "writing slices to " << slicesFile << std::endl;

	std::ofstream out(slicesFile.c_str());

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		if (end->getDirection() == Left)
			writeSlice(*end->getSlice(), out);
	}

	LOG_DEBUG(problemgraphwriterlog) << "writing slice imagess to " << sliceImageDirectory << std::endl;

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		if (end->getDirection() == Left)
			writeSliceImage(*end->getSlice(), sliceImageDirectory);
	}
	// TODO
}

void
ProblemGraphWriter::writeSegments(const std::string& segmentsFile) {

	LOG_DEBUG(problemgraphwriterlog) << "writing segments to " << segmentsFile << std::endl;

	std::ofstream out(segmentsFile.c_str());

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		writeSegment(*end, out);

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations())
		writeSegment(*continuation, out);

	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches())
		writeSegment(*branch, out);

	out.close();
}

void
ProblemGraphWriter::writeConstraints() {

	LOG_DEBUG(problemgraphwriterlog) << "writing constraints" << std::endl;
}

void
ProblemGraphWriter::writeSlice(const Slice& slice, std::ofstream& out) {

	LOG_DEBUG(problemgraphwriterlog) << "writing slices" << std::endl;

	// TODO: write whatever you want to know about a slice into out

	//slice.getComponent()...
}

void
ProblemGraphWriter::writeSliceImage(const Slice& slice, const std::string& sliceImageDirectory) {

	unsigned int section = slice.getSection();
	unsigned int id      = slice.getId();

	std::string filename = sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section) + "_" + boost::lexical_cast<std::string>(id) + ".png";

	vigra::exportImage(vigra::srcImageRange(slice.getComponent()->getBitmap()), vigra::ImageExportInfo(filename.c_str()));
}

void
ProblemGraphWriter::writeSegment(const Segment& segment, std::ofstream& out) {

	LOG_ALL(problemgraphwriterlog) << "writing segment " << segment.getId() << std::endl;

	out << segment.getId() << " ";

	out << segment.getSlices().size();

	foreach (boost::shared_ptr<Slice> slice, segment.getSlices()) {

		out << " " << slice->getId();
	}

	out << " " << _objective->getCoefficients()[(*_segmentIdsToVariables)[segment.getId()]];

	out << std::endl;
}
