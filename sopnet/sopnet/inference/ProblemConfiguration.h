#ifndef SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__
#define SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__

#include <boost/lexical_cast.hpp>

#include <pipeline/all.h>
#include <sopnet/exceptions.h>

class ProblemConfiguration : public pipeline::Data {

public:

	void setVariable(unsigned int segmentId, unsigned int variable) {

		_variables[segmentId] = variable;
		_segmentIds[variable] = segmentId;
	}

	unsigned int getVariable(unsigned int segmentId) {

		if (!_variables.count(segmentId))
			BOOST_THROW_EXCEPTION(
					NoSuchSegment()
					<< error_message(
							std::string("variable map does not contain an entry for segment id ") +
							boost::lexical_cast<std::string>(segmentId))
					<< STACK_TRACE);

		return _variables[segmentId];
	}

	unsigned int getSegmentId(unsigned int variable) {

		if (!_segmentIds.count(variable))
			BOOST_THROW_EXCEPTION(
					NoSuchSegment()
					<< error_message(
							std::string("segment id map does not contain an entry for variable ") +
							boost::lexical_cast<std::string>(variable))
					<< STACK_TRACE);

		return _segmentIds[variable];
	}

	void clear() {

		_variables.clear();
		_segmentIds.clear();
	}

private:

	// mapping of segment ids to a continous range of variable numbers
	std::map<unsigned int, unsigned int> _variables;

	// reverse mapping
	std::map<unsigned int, unsigned int> _segmentIds;
};

#endif // SOPNET_INFERENCE_PROBLEM_CONFIGURATION_H__

