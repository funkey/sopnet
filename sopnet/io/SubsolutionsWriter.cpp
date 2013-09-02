#include <fstream>

#include "SubsolutionsWriter.h"

SubsolutionsWriter::SubsolutionsWriter(const std::string& stream) :
	_streamName(stream),
	_fb(0),
	_stream(0) {

	if (writeStdOut()) {

		_stream = new std::ostream(std::cout.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(stream.c_str(), std::ios::out);

		_stream = new std::ostream(_fb);
	}

	registerInput(_subsolutions, "subsolutions");
	registerInput(_subproblems,  "subproblems");
}

SubsolutionsWriter::SubsolutionsWriter(const SubsolutionsWriter& other) {

	free();
	copy(other);
}

SubsolutionsWriter&
SubsolutionsWriter::operator=(const SubsolutionsWriter& other) {

	free();
	copy(other);

	return *this;
}

SubsolutionsWriter::~SubsolutionsWriter() {

	free();
}

void
SubsolutionsWriter::free() {

	if (_stream) {

		delete _stream;
		_stream = 0;
	}

	if (_fb && !writeStdOut()) {

		delete _fb;
		_fb = 0;
	}
}

void
SubsolutionsWriter::copy(const SubsolutionsWriter& other) {

	if (other.writeStdOut()) {

		_stream = new std::ostream(std::cout.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(other._streamName.c_str(), std::ios::out);

		_stream = new std::ostream(_fb);
	}
}

bool
SubsolutionsWriter::writeStdOut() const {

	return (_streamName == "-");
}

void
SubsolutionsWriter::write() {

	updateInputs();

	*_stream << _subsolutions->size() << std::endl;

	for (int i = 0; i < _subsolutions->size(); i++)
		writeSubsolution(*(_subsolutions->getSolution(i)), *(_subproblems->getProblem(i)->getConfiguration()));
}

void
SubsolutionsWriter::writeSubsolution(const Solution& solution, ProblemConfiguration& configuration) {

	unsigned int numSegments = 0;
	for (int i = 0; i < solution.size(); i++)
		if (solution[i] == 1)
			numSegments++;

	*_stream << numSegments;

	for (int i = 0; i < solution.size(); i++) {

		if (solution[i] == 1)
			*_stream << " " << configuration.getSegmentId(i);
	}
}
