#include <util/foreach.h>
#include "LinearConstraint.h"

LinearConstraint::LinearConstraint() :
	_relation(LessEqual) {}

void
LinearConstraint::setCoefficient(unsigned int varNum, double coef) {

	if (coef == 0) {

		_coefs.erase(_coefs.find(varNum));

	} else {

		_coefs[varNum] = coef;
	}
}

void
LinearConstraint::setRelation(Relation relation) {

	_relation = relation;
}

void
LinearConstraint::setValue(double value) {

	_value = value;
}

const std::map<unsigned int, double>&
LinearConstraint::getCoefficients() const {

	return _coefs;
}

const Relation&
LinearConstraint::getRelation() const {

	return _relation;
}

double
LinearConstraint::getValue() const {

	return _value;
}

std::ostream& operator<<(std::ostream& out, const LinearConstraint& constraint) {

	typedef std::map<unsigned int, double>::value_type pair_t;
	foreach (const pair_t& pair, constraint.getCoefficients())
		out << pair.second << "*" << pair.first << " ";

	out << (constraint.getRelation() == LessEqual ? "<=" : (constraint.getRelation() == GreaterEqual ? ">=" : "=="));

	out << " " << constraint.getValue();

	return out;
}
