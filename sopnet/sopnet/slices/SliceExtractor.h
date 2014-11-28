#ifndef SOPNET_SLICE_EXTRACTOR_H__
#define SOPNET_SLICE_EXTRACTOR_H__

#include <deque>

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <imageprocessing/ComponentTreeExtractor.h>
#include "Slices.h"

// forward declaration
class ComponentTreeDownSampler;
class ComponentTreePruner;
class ComponentTreeConverter;
template <typename Precision> class ComponentTreeExtractor;

template <typename Precision>
class SliceExtractor : public pipeline::ProcessNode {

public:

	/**
	 * Create a new slice extractor for the given section.
	 *
	 * @param section
	 *              The section number that the extracted slices will have.
	 *
	 * @param resX, resY, resZ
	 *              The resolution of the image stack used to extract slices.
	 *
	 * @param downsample
	 *              Do not extract slices that are single children of their 
	 *              parents in the component tree.
	 */
	SliceExtractor(unsigned int section, float resX, float resY, float resZ, bool downsample);

private:

	void onInputSet(const pipeline::InputSetBase& signal);

	// optional cte parameters to override the program options
	pipeline::Input<ComponentTreeExtractorParameters> _cteParameters;

	void extractSlices();

	boost::shared_ptr<ComponentTreeExtractor<Precision> > _cte;
	boost::shared_ptr<ComponentTreeExtractorParameters>   _defaultCteParameters;
	boost::shared_ptr<ComponentTreeDownSampler> _downSampler;
	boost::shared_ptr<ComponentTreePruner>      _pruner;
	boost::shared_ptr<ComponentTreeConverter>   _converter;
};

#endif // SOPNET_SLICE_EXTRACTOR_H__

