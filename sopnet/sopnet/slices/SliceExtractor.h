#ifndef SOPNET_SLICE_EXTRACTOR_H__
#define SOPNET_SLICE_EXTRACTOR_H__

#include <deque>

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/MserParameters.h>
#include "Slices.h"

// forward declaration
class ComponentTreeDownSampler;
class ComponentTreePruner;
class ComponentTreeConverter;
template <typename Precision> class Mser;
class MserParameters;

template <typename Precision>
class SliceExtractor : public pipeline::ProcessNode {

public:

	SliceExtractor(unsigned int section);

private:

	// modifies the linear constraints according to the parameter 'force explanation'
	class LinearConstraintsFilter : public pipeline::SimpleProcessNode<> {

	public:

		LinearConstraintsFilter();

	private:

		void updateOutputs();

		pipeline::Input<LinearConstraints>  _linearConstraints;
		pipeline::Input<bool>               _forceExplanation;
		pipeline::Output<LinearConstraints> _filtered;
	};

	void onInputSet(const pipeline::InputSetBase& signal);

	// optional mser parameters to override the program options
	pipeline::Input<MserParameters> _mserParameters;

	void extractSlices();

	boost::shared_ptr<Mser<Precision> >         _mser;
	boost::shared_ptr<MserParameters>           _defaultMserParameters;
	boost::shared_ptr<ComponentTreeDownSampler> _downSampler;
	boost::shared_ptr<ComponentTreePruner>      _pruner;
	boost::shared_ptr<ComponentTreeConverter>   _converter;
	boost::shared_ptr<LinearConstraintsFilter>  _filter;
};

#endif // SOPNET_SLICE_EXTRACTOR_H__

