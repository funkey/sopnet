#include <fstream>

#include <util/Logger.h>

#include "SubproblemsReader.h"

logger::LogChannel streamproblemreaderlog("streamproblemreaderlog", "[SubproblemsReader] ");

SubproblemsReader::SubproblemsReader(const std::string& stream) :
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

	registerOutput(_subproblems, "subproblems");
}

SubproblemsReader::SubproblemsReader(const SubproblemsReader& other) {

	free();
	copy(other);
}

SubproblemsReader&
SubproblemsReader::operator=(const SubproblemsReader& other) {

	free();
	copy(other);

	return *this;
}

SubproblemsReader::~SubproblemsReader() {

	free();
}

void
SubproblemsReader::free() {

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
SubproblemsReader::copy(const SubproblemsReader& other) {

	if (other.readStdIn()) {

		_stream = new std::istream(std::cin.rdbuf());

	} else {

		_fb = new std::filebuf;
		_fb->open(other._streamName.c_str(), std::ios::in);

		_stream = new std::istream(_fb);
	}
}

bool
SubproblemsReader::readStdIn() const {

	return (_streamName == "-");
}

void
SubproblemsReader::updateOutputs() {

	// clear existing subproblems
	_subproblems->clear();

	unsigned int numSubproblems;
	*_stream >> numSubproblems;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numSubproblems << " subproblems" << std::endl;

	for (unsigned int i = 0; i < numSubproblems; i++)
		readSubproblem(i);
}

void
SubproblemsReader::readSubproblem(unsigned int numSubproblem) {

	unsigned int numVariables;
	*_stream >> numVariables;

	// create a new problem
	boost::shared_ptr<Problem> problem = boost::make_shared<Problem>(numVariables);

	// add it to the existing subproblems
	_subproblems->addProblem(problem);

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numVariables << " variables" << std::endl;

	for (unsigned int i = 0; i < numVariables; i++)
		readVariable(*problem, i);

	unsigned int numOneConstraints;
	*_stream >> numOneConstraints;

	char rel[2];
	*_stream >> rel;

	Relation relation;
	if (rel[0] == '<')
		relation = LessEqual;
	else
		relation = Equal;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numOneConstraints << " one-constraints with relation " << rel << std::endl;

	for (unsigned int i = 0; i < numOneConstraints; i++)
		readOneConstraint(*problem, i, relation);

	unsigned int numEqualConstraints;
	*_stream >> numEqualConstraints;

	LOG_DEBUG(streamproblemreaderlog) << "reading " << numEqualConstraints << " equal-constraints" << std::endl;

	for (unsigned int i = 0; i < numEqualConstraints; i++)
		readEqualConstraint(*problem, i);
}

void
SubproblemsReader::readVariable(Problem& problem, unsigned int i) {

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
SubproblemsReader::readOneConstraint(Problem& problem, unsigned int i, Relation relation) {

	LinearConstraint constraint;

	unsigned int numVariables;

	*_stream >> numVariables;

	for (int j = 0; j < numVariables; j++) {

		unsigned int id;
		*_stream >> id;

		unsigned int varNum = problem.getConfiguration()->getVariable(id);

		constraint.setCoefficient(varNum, 1.0);
	}

	constraint.setRelation(relation);
	constraint.setValue(1.0);

	problem.getLinearConstraints()->add(constraint);

	LOG_ALL(streamproblemreaderlog) << "found constraint " << constraint << std::endl;
}

void
SubproblemsReader::readEqualConstraint(Problem& problem, unsigned int i) {

	LinearConstraint constraint;

	unsigned int numVariables;

	*_stream >> numVariables;

	for (int j = 0; j < numVariables; j++) {

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

	constraint.setRelation(Equal);
	constraint.setValue(0.0);

	problem.getLinearConstraints()->add(constraint);

	LOG_ALL(streamproblemreaderlog) << "found constraint " << constraint << std::endl;
}
