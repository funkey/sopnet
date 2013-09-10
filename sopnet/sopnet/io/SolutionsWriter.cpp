#include <fstream>

#include "SolutionsWriter.h"

SolutionsWriter::SolutionsWriter(const std::string& stream) :
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

	registerInput(_solutions, "solutions");
	registerInput(_problems,  "problems");
}

SolutionsWriter::SolutionsWriter(const SolutionsWriter& other) {

	free();
	copy(other);
}

SolutionsWriter&
SolutionsWriter::operator=(const SolutionsWriter& other) {

	free();
	copy(other);

	return *this;
}

SolutionsWriter::~SolutionsWriter() {

	free();
}

void
SolutionsWriter::free() {

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
SolutionsWriter::copy(const SolutionsWriter& other) {

	if (other.writeStdOut()) {

		_stream = new std::ostream(std::cout.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(other._streamName.c_str(), std::ios::out);

		_stream = new std::ostream(_fb);
	}
}

bool
SolutionsWriter::writeStdOut() const {

	return (_streamName == "-");
}

void
SolutionsWriter::write() {

	updateInputs();

	*_stream << _solutions->size() << std::endl;

	for (int i = 0; i < _solutions->size(); i++)
		writeSolution(*(_solutions->getSolution(i)), *(_problems->getProblem(i)->getConfiguration()));
}

void
SolutionsWriter::writeSolution(const Solution& solution, ProblemConfiguration& configuration) {

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
