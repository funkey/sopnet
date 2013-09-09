#ifndef SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__
#define SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__

#include <boost/lexical_cast.hpp>

#include <pipeline/all.h>
#include <sopnet/exceptions.h>
#include <sopnet/segments/Segments.h>

class ProblemConfiguration : public pipeline::Data {

public:

	ProblemConfiguration();

	/**
	 * Assign a segment to a variable id. Remembers the inter-section interval 
	 * and the extends of the problem.
	 */
	void setVariable(const Segment& segment, unsigned int variable);

	/**
	 * Assign a segment id to a variable id.
	 */
	void setVariable(unsigned int segmentId, unsigned int variable);

	/**
	 * Get the variable id from a segment id.
	 */
	unsigned int getVariable(unsigned int segmentId);

	/**
	 * Get the segment id from a variable id.
	 */
	unsigned int getSegmentId(unsigned int variable);

	/**
	 * Get the inter-section interval that corresponds to a variable.
	 */
	unsigned int getInterSectionInterval(unsigned int variable) { return _interSectionIntervals[variable]; }

	unsigned int getMinInterSectionInterval() { return _minInterSectionInterval; }
	unsigned int getMaxInterSectionInterval() { return _minInterSectionInterval; }
	unsigned int getMinX() { return _minX; }
	unsigned int getMaxX() { return _minX; }
	unsigned int getMinY() { return _minY; }
	unsigned int getMaxY() { return _minY; }

	/**
	 * Get all the variables that are assigned to the intersection intervals 
	 * between (including) minInterSectionInterval and (excluding) 
	 * maxInterSectionInterval.
	 */
	std::vector<unsigned int> getVariables(unsigned int minInterSectionInterval, unsigned int maxInterSectionInterval);

	/**
	 * Clear the mapping.
	 */
	void clear();

private:

	void fit(const Segment& segment);

	// mapping of segment ids to a continous range of variable numbers
	std::map<unsigned int, unsigned int> _variables;

	// reverse mapping
	std::map<unsigned int, unsigned int> _segmentIds;

	// mapping from variable ids to inter-section intervals
	std::map<unsigned int, unsigned int> _interSectionIntervals;

	// the boundary of the problem in volume space
	int _minInterSectionInterval;
	int _maxInterSectionInterval;
	int _minX, _maxX;
	int _minY, _maxY;
};

#endif // SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__

