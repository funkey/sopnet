#include <util/ProgramOptions.h>
#include <imageprocessing/ComponentTreeExtractor.h>
#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/ComponentTreePruner.h>
#include <pipeline/Value.h>
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

util::ProgramOption optionMaxSliceMerges(
		util::_module           = "sopnet",
		util::_long_name        = "maxSliceMerges",
		util::_description_text = "Limit the height of the slice component tree, counting the height from the leafs.",
		util::_default_value    = 3);

template <typename Precision>
SliceExtractor<Precision>::SliceExtractor(unsigned int section, float resX, float resY, float resZ, bool downsample) :
	_cte(boost::make_shared<ComponentTreeExtractor<Precision> >()),
	_defaultCteParameters(boost::make_shared<ComponentTreeExtractorParameters>()),
	_downSampler(boost::make_shared<ComponentTreeDownSampler>()),
	_pruner(boost::make_shared<ComponentTreePruner>()),
	_converter(boost::make_shared<ComponentTreeConverter>(section, resX, resY, resZ)) {

	registerInput(_cte->getInput("image"), "membrane");
	registerInput(_cteParameters, "component tree extractor parameters");
	registerOutput(_converter->getOutput("slices"), "slices");
	registerOutput(_converter->getOutput("conflict sets"), "conflict sets");

	_cteParameters.registerCallback(&SliceExtractor<Precision>::onInputSet, this);

	// set default cte parameters from program options
	_defaultCteParameters->darkToBright =  optionInvertSliceMaps;
	_defaultCteParameters->minSize      =  optionMinSliceSize;
	_defaultCteParameters->maxSize      =  optionMaxSliceSize;

	LOG_DEBUG(sliceextractorlog)
			<< "extracting slices with min size " << optionMinSliceSize.as<int>()
			<< ", max size " << optionMaxSliceSize.as<int>()
			<< ", and max tree depth " << optionMaxSliceMerges.as<int>()
			<< std::endl;

	// setup internal pipeline
	_cte->setInput("parameters", _defaultCteParameters);

	if (downsample) {

		_downSampler->setInput(_cte->getOutput());
		_pruner->setInput("component tree", _downSampler->getOutput());

	} else {

		_pruner->setInput("component tree", _cte->getOutput());
	}
	_pruner->setInput("max height", pipeline::Value<int>(optionMaxSliceMerges.as<int>()));
	_converter->setInput(_pruner->getOutput());
}

template <typename Precision>
void
SliceExtractor<Precision>::onInputSet(const pipeline::InputSetBase&) {

	LOG_ALL(sliceextractorlog) << "using non-default component-tree-extractor parameters" << std::endl;

	// don't use the default
	_cte->setInput("parameters", _cteParameters);
}

// explicit template instantiations
template class SliceExtractor<unsigned char>;
template class SliceExtractor<unsigned short>;
