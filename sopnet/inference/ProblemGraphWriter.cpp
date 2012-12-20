#include <fstream>

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

	writeSlices();

	writeSegments(segmentsFile);

	writeConstraints();

	LOG_DEBUG(problemgraphwriterlog) << "done" << std::endl;
}

void
ProblemGraphWriter::writeSlices() {

	LOG_DEBUG(problemgraphwriterlog) << "writing slices" << std::endl;

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		if (end->getDirection() == Left)
			writeSlice(*end->getSlice());
	}
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
ProblemGraphWriter::writeSlice(const Slice& slice) {

}

void
ProblemGraphWriter::writeSegment(const Segment& segment, std::ofstream& out) {

	LOG_ALL(problemgraphwriterlog) << "writing segment " << segment.getId() << std::endl;

	out << segment.getSlices().size();

	foreach (boost::shared_ptr<Slice> slice, segment.getSlices()) {

		out << " " << slice->getId();
	}

	out << " " << _objective->getCoefficients()[(*_segmentIdsToVariables)[segment.getId()]];

	out << std::endl;
}
