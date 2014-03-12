#include <fstream>

#include <util/Logger.h>

#include "ProblemsReader.h"

logger::LogChannel streamproblemreaderlog("problemsreaderlog", "[ProblemsReader] ");

ProblemsReader::ProblemsReader(const std::string& stream) :
	_problems(new Problems()),
	_streamName(stream),
	_stream(0),
	_fb(0) {

	if (readStdIn()) {

		_stream = new std::istream(std::cin.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(stream.c_str(), std::ios::in);

		_stream = new std::istream(_fb);
	}

	registerOutput(_problems, "problems");
}

ProblemsReader::ProblemsReader(const ProblemsReader& other) :
	SimpleProcessNode<>() {

	free();
	copy(other);
}

ProblemsReader&
ProblemsReader::operator=(const ProblemsReader& other) {

	free();
	copy(other);

	return *this;
}

ProblemsReader::~ProblemsReader() {

	free();
}

void
ProblemsReader::free() {

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
ProblemsReader::copy(const ProblemsReader& other) {

	if (other.readStdIn()) {

		_stream = new std::istream(std::cin.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(other._streamName.c_str(), std::ios::in);

		_stream = new std::istream(_fb);
	}
}

bool
ProblemsReader::readStdIn() const {

	return (_streamName == "-");
}

void
ProblemsReader::updateOutputs() {

	// clear existing problems
	_problems->clear();

	unsigned int numProblems;
	*_stream >> numProblems;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numProblems << " problems" << std::endl;

	for (unsigned int i = 0; i < numProblems; i++)
		readProblem(i);
}

void
ProblemsReader::readProblem(unsigned int /*numProblem*/) {

	unsigned int numVariables;
	*_stream >> numVariables;

	// create a new problem
	boost::shared_ptr<Problem> problem = boost::make_shared<Problem>(numVariables);

	// add it to the existing problems
	_problems->addProblem(problem);

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numVariables << " variables" << std::endl;

	for (unsigned int i = 0; i < numVariables; i++)
		readVariable(*problem, i);

	unsigned int numConstraints;
	*_stream >> numConstraints;

	for (unsigned int i = 0; i < numConstraints; i++)
		readConstraint(*problem, i);
}

void
ProblemsReader::readVariable(Problem& problem, unsigned int i) {

	unsigned int id;
	float costs;

	*_stream >> id;
	*_stream >> costs;

	LOG_ALL(streamproblemreaderlog) << "found variable " << i << " (id " << id << ") with costs " << costs << std::endl;

	// set costs in objective
	problem.getObjective()->setCoefficient(i, costs);

	// remember mapping from id to variable num
	problem.getConfiguration()->setVariable(id, i);
}

void
ProblemsReader::readConstraint(Problem& problem, unsigned int /*i*/) {

	LinearConstraint constraint;

	double       value;
	char         rel[2];
	unsigned int numVariables;

	*_stream >> value >> rel >> numVariables;

	Relation relation;
	if (rel[0] == '<')
		relation = GreaterEqual; // switch relation, since in file: value rel term
	else if (rel[0] == '>')
		relation = LessEqual; // switch relation, since in file: value rel term
	else
		relation = Equal;

	constraint.setRelation(relation);
	constraint.setValue(value);

	for (unsigned int j = 0; j < numVariables; j++) {

		int id;
		*_stream >> id;

		if (id >= 0) {

			unsigned int varNum = problem.getConfiguration()->getVariable(id);
			constraint.setCoefficient(varNum, 1.0);

		} else {

			unsigned int varNum = problem.getConfiguration()->getVariable(-id);
			constraint.setCoefficient(varNum, -1.0);
		}
	}

	problem.getLinearConstraints()->add(constraint);

	LOG_ALL(streamproblemreaderlog) << "found constraint " << constraint << std::endl;
}
