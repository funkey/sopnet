#ifndef INFERENCE_LINEAR_CONSTRAINTS_H__
#define INFERENCE_LINEAR_CONSTRAINTS_H__

#include <pipeline/all.h>
#include "LinearConstraint.h"

class LinearConstraints : public pipeline::Data {

	typedef std::vector<LinearConstraint> linear_constraints_type;

public:

	typedef linear_constraints_type::iterator       iterator;

	typedef linear_constraints_type::const_iterator const_iterator;

	/**
	 * Create a new set of linear constraints and allocate enough memory to hold
	 * 'size' linear constraints. More or less constraints can be added, but
	 * memory might be wasted (if more allocated then necessary) or unnecessary
	 * reallocations might occur (if more added than allocated).
	 *
	 * @param size The number of linear constraints to reserve memory for.
	 */
	LinearConstraints(size_t size = 0);

	/**
	 * Remove all constraints from this set of linear constraints.
	 */
	void clear() { _linearConstraints.clear(); }

	/**
	 * Add a linear constraint.
	 *
	 * @param linearConstraint The linear constraint to add.
	 */
	void add(const LinearConstraint& linearConstraint);

	/**
	 * Add a set of linear constraints.
	 *
	 * @param linearConstraints The set of linear constraints to add.
	 */
	void addAll(const LinearConstraints& linearConstraints);

	/**
	 * @return The number of linear constraints in this set.
	 */
	unsigned int size() const { return _linearConstraints.size(); }

	const const_iterator begin() const { return _linearConstraints.begin(); }

	iterator begin() { return _linearConstraints.begin(); }

	const const_iterator end() const { return _linearConstraints.end(); }

	iterator end() { return _linearConstraints.end(); }

	const LinearConstraint& operator[](size_t i) const { return _linearConstraints[i]; }

	LinearConstraint& operator[](size_t i) { return _linearConstraints[i]; }

	/**
	 * Get a linst of linear constraints that use the given variables.
	 */
	LinearConstraints getConstraints(const std::vector<unsigned int>& variableIds);

private:

	linear_constraints_type _linearConstraints;
};

#endif // INFERENCE_LINEAR_CONSTRAINTS_H__

