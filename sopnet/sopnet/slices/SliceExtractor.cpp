#include <util/ProgramOptions.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/Mser.h>
#include "ComponentTreeConverter.h"
#include "SliceExtractor.h"

static logger::LogChannel sliceextractorlog("sliceextractorlog", "[SliceExtractor] ");

util::ProgramOption optionInvertSliceMaps(
		util::_module           = "sopnet",
		util::_long_name        = "invertSliceMaps",
		util::_description_text = "Invert the meaning of the slice map. The default "
		                          "(not inverting) is: bright area = neuron hypotheses.");

util::ProgramOption optionMinSliceSize(
		util::_module           = "sopnet",
		util::_long_name        = "minSliceSize",
		util::_description_text = "The minimal size of a neuron slice in pixels.",
		util::_default_value    = 10);

util::ProgramOption optionMaxSliceSize(
		util::_module           = "sopnet",
		util::_long_name        = "maxSliceSize",
		util::_description_text = "The maximal size of a neuron slice in pixels.",
		util::_default_value    = 100*100);

template <typename Precision>
SliceExtractor<Precision>::SliceExtractor(unsigned int section) :
	_mser(boost::make_shared<Mser<Precision> >()),
	_defaultMserParameters(boost::make_shared<MserParameters>()),
	_downSampler(boost::make_shared<ComponentTreeDownSampler>()),
	_converter(boost::make_shared<ComponentTreeConverter>(section)),
	_filter(boost::make_shared<LinearConstraintsFilter>()) {

	registerInput(_mser->getInput("image"), "membrane");
	registerInput(_mserParameters, "mser parameters");
	registerInput(_filter->getInput("force explanation"), "force explanation");
	registerOutput(_converter->getOutput("slices"), "slices");
	registerOutput(_filter->getOutput("linear constraints"), "linear constraints");

	_mserParameters.registerBackwardCallback(&SliceExtractor<Precision>::onInputSet, this);

	// set default mser parameters from program options
	_defaultMserParameters->darkToBright =  optionInvertSliceMaps;
	_defaultMserParameters->brightToDark = !optionInvertSliceMaps;
	_defaultMserParameters->minArea      =  optionMinSliceSize;
	_defaultMserParameters->maxArea      =  optionMaxSliceSize;

	// setup internal pipeline
	_mser->setInput("parameters", _defaultMserParameters);
	_downSampler->setInput(_mser->getOutput());
	_converter->setInput(_downSampler->getOutput());
	_filter->setInput("linear constraints", _converter->getOutput("linear constraints"));
}

template <typename Precision>
void
SliceExtractor<Precision>::onInputSet(const pipeline::InputSetBase&) {

	LOG_ALL(sliceextractorlog) << "using non-default mser parameters" << std::endl;

	// don't use the default
	_mser->setInput("parameters", _mserParameters);
}

template <typename Precision>
SliceExtractor<Precision>::LinearConstraintsFilter::LinearConstraintsFilter() {

	registerInput(_linearConstraints, "linear constraints");
	registerInput(_forceExplanation, "force explanation");
	registerOutput(_filtered, "linear constraints");
}

template <typename Precision>
void
SliceExtractor<Precision>::LinearConstraintsFilter::updateOutputs() {

	*_filtered = *_linearConstraints;

	foreach (LinearConstraint& linearConstraint, *_filtered) {

		if (_forceExplanation && *_forceExplanation)
			linearConstraint.setRelation(Equal);
		else
			linearConstraint.setRelation(LessEqual);
	}
}

// explicit template instantiations
template class SliceExtractor<unsigned char>;
template class SliceExtractor<unsigned short>;
