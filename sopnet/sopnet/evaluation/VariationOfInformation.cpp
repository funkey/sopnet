#include <util/Logger.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include "VariationOfInformation.h"

util::ProgramOption optionVoiIgnoreBackground(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "voiIgnoreBackground",
		util::_description_text = "For the computation of the VOI, do not consider background pixels in the ground truth.");

logger::LogChannel variationofinformationlog("variationofinformationlog", "[ResultEvaluator] ");

VariationOfInformation::VariationOfInformation() :
		_ignoreBackground(optionVoiIgnoreBackground.as<bool>()) {

	registerInput(_stack1, "stack 1");
	registerInput(_stack2, "stack 2");
	registerOutput(_errors, "errors");
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

			if (_ignoreBackground && (*j1 == 0 || *j2 == 0)) {

				n--;
				continue;
			}

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

	// H(stack 1)
	double H1 = 0.0;
	// H(stack 2)
	double H2 = 0.0;
	double I  = 0.0;

	for(typename LabelProb::const_iterator i = _p1.begin(); i != _p1.end(); i++)
		H1 -= i->second * std::log(i->second);

	for(typename LabelProb::const_iterator i = _p2.begin(); i != _p2.end(); i++)
		H2 -= i->second * std::log(i->second);

	for(typename JointLabelProb::const_iterator i = _p12.begin(); i != _p12.end(); i++) {

		const float j = i->first.first;
		const float k = i->first.second;

		const double pjk = i->second;
		const double pj  = _p1[j];
		const double pk  = _p2[k];

		I += pjk * std::log( pjk / (pj*pk) );
	}

	// H(stack 1, stack2)
	double H12 = H1 + H2 - I;

	// set output
	_errors = new VariationOfInformationErrors();

	// We compare stack1 to stack2. Thus, the split entropy represents the 
	// number of splits from stack1 to stack2, and the merge entropy the number 
	// of merges from stack1 to stack2.
	//
	// H(stack 2|stack 1) = H(stack 1, stack 2) - H(stack 1)
	_errors->setSplitEntropy(H12 - H1);
	// H(stack 1|stack 2) = H(stack 1, stack 2) - H(stack 2)
	_errors->setMergeEntropy(H12 - H2);

	LOG_DEBUG(variationofinformationlog)
			<< "sum of conditional entropies is " << _errors->getEntropy()
			<< ", which should be equal to " << (H1 + H2 - 2.0*I) << std::endl;
}
