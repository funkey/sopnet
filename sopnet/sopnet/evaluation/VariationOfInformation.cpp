#include <util/Logger.h>
#include <util/exceptions.h>
#include "VariationOfInformation.h"

logger::LogChannel variationofinformationlog("variationofinformationlog", "[ResultEvaluator] ");

VariationOfInformation::VariationOfInformation() {

	registerInput(_stack1, "stack 1");
	registerInput(_stack2, "stack 2");
	registerOutput(_variationOfInformation, "variation of information");
}

void
VariationOfInformation::updateOutputs() {

	if (_stack1->size() != _stack2->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("image stacks have different size") << STACK_TRACE);

	// count label occurences

	_p1.clear();
	_p2.clear();
	_p12.clear();

	ImageStack::const_iterator i1  = _stack1->begin();
	ImageStack::const_iterator i2  = _stack2->begin();

	unsigned int n = 0;

	for (; i1 != _stack1->end(); i1++, i2++) {

		if ((*i1)->size() != (*i2)->size())
			BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("images have different size") << STACK_TRACE);

		n += (*i1)->size();

		Image::iterator j1 = (*i1)->begin();
		Image::iterator j2 = (*i2)->begin();

		for (; j1 != (*i1)->end(); j1++, j2++) {

			_p1[*j1]++;
			_p2[*j2]++;
			_p12[std::make_pair(*j1, *j2)]++;
		}
	}

	// normalize

	for (typename LabelProb::iterator i = _p1.begin(); i != _p1.end(); i++)
		i->second /= n;
	for (typename LabelProb::iterator i = _p2.begin(); i != _p2.end(); i++)
		i->second /= n;
	for (typename JointLabelProb::iterator i = _p12.begin(); i != _p12.end(); i++)
		i->second /= n;

	// compute information

	double H0 = 0.0;
	double H1 = 0.0;
	double I  = 0.0;

	for(typename LabelProb::const_iterator i = _p1.begin(); i != _p1.end(); i++)
		H0 -= i->second * std::log(i->second);

	for(typename LabelProb::const_iterator i = _p2.begin(); i != _p2.end(); i++)
		H1 -= i->second * std::log(i->second);

	for(typename JointLabelProb::const_iterator i = _p12.begin(); i != _p12.end(); i++) {

		const float j = i->first.first;
		const float k = i->first.second;

		const double pjk = i->second;
		const double pj  = _p1[j];
		const double pk  = _p2[k];

		I += pjk * std::log( pjk / (pj*pk) );
	}

	// set output

	*_variationOfInformation = H0 + H1 - 2.0 * I;

	// dump to output (useful for redirection into file)
	LOG_USER(variationofinformationlog) << "# VOI" << std::endl;
	LOG_USER(variationofinformationlog) << (*_variationOfInformation) << std::endl;
}
