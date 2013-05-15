#include <fstream>

#include <util/Logger.h>

#include "StreamProblemReader.h"

logger::LogChannel streamproblemreaderlog("streamproblemreaderlog", "[StreamProblemReader] ");

StreamProblemReader::StreamProblemReader(const std::string& stream) :
	_streamName(stream),
	_fb(0),
	_stream(0) {

	if (readStdIn()) {

		_stream = new std::istream(std::cin.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(stream.c_str(), std::ios::in);

		_stream = new std::istream(_fb);
	}

	registerOutput(_problems, "problems");
}

StreamProblemReader::StreamProblemReader(const StreamProblemReader& other) {

	free();
	copy(other);
}

StreamProblemReader&
StreamProblemReader::operator=(const StreamProblemReader& other) {

	free();
	copy(other);

	return *this;
}

StreamProblemReader::~StreamProblemReader() {

	free();
}

void
StreamProblemReader::free() {

	if (_stream) {

		delete _stream;
		_stream = 0;
	}

	if (_fb && !readStdIn()) {

		delete _fb;
		_fb = 0;
	}
}

void
StreamProblemReader::copy(const StreamProblemReader& other) {

	if (other.readStdIn()) {

		_stream = new std::istream(std::cin.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(other._streamName.c_str(), std::ios::in);

		_stream = new std::istream(_fb);
	}
}

bool
StreamProblemReader::readStdIn() const {

	return (_streamName == "-");
}

void
StreamProblemReader::updateOutputs() {

	unsigned int numVariables;
	*_stream >> numVariables;

	// create a new problem

	boost::shared_ptr<Problem> problem = boost::make_shared<Problem>(numVariables);
	_problems->clear();
	_problems->addProblem(problem);

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numVariables << " variables" << std::endl;

	for (unsigned int i = 0; i < numVariables; i++)
		readVariable(i);

	unsigned int numOneConstraints;
	*_stream >> numOneConstraints;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numOneConstraints << " one-constraints" << std::endl;

	for (unsigned int i = 0; i < numOneConstraints; i++)
		readOneConstraint(i);

	unsigned int numEqualConstraints;
	*_stream >> numEqualConstraints;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numEqualConstraints << " equal-constraints" << std::endl;

	for (unsigned int i = 0; i < numEqualConstraints; i++)
		readEqualConstraint(i);
}

void
StreamProblemReader::readVariable(unsigned int i) {

	unsigned int id;
	float costs;

	*_stream >> id;
	*_stream >> costs;

	LOG_ALL(streamproblemreaderlog) << "found variable " << i << " (id " << id << ") with costs " << costs << std::endl;

	// set costs in objective
	_problems->getProblem(0)->getObjective().setCoefficient(i, costs);

	// remember mapping from id to variable num
	_problems->getProblem(0)->getConfiguration().setVariable(id, i);
}

void
StreamProblemReader::readOneConstraint(unsigned int i) {

	LinearConstraint constraint;

	unsigned int numVariables;

	*_stream >> numVariables;

	for (int j = 0; j < numVariables; j++) {

		unsigned int id;
		*_stream >> id;

		unsigned int varNum = _problems->getProblem(0)->getConfiguration().getVariable(id);

		constraint.setCoefficient(varNum, 1.0);
	}

	constraint.setRelation(LessEqual);
	constraint.setValue(1.0);

	LOG_ALL(streamproblemreaderlog) << "found constraint " << constraint << std::endl;
}

void
StreamProblemReader::readEqualConstraint(unsigned int i) {

	LinearConstraint constraint;

	unsigned int numVariables;

	*_stream >> numVariables;

	for (int j = 0; j < numVariables; j++) {

		int id;
		*_stream >> id;

		if (id >= 0) {

			unsigned int varNum = _problems->getProblem(0)->getConfiguration().getVariable(id);
			constraint.setCoefficient(varNum, 1.0);

		} else {

			unsigned int varNum = _problems->getProblem(0)->getConfiguration().getVariable(-id);
			constraint.setCoefficient(varNum, -1.0);
		}
	}

	constraint.setRelation(Equal);
	constraint.setValue(0.0);

	LOG_ALL(streamproblemreaderlog) << "found constraint " << constraint << std::endl;
}
