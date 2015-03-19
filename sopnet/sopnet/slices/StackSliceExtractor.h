#ifndef SOPNET_STACK_SLICE_EXTRACTOR_H__
#define SOPNET_STACK_SLICE_EXTRACTOR_H__

#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/ComponentTreeExtractor.h>
#include "ConflictSets.h"
#include "Slices.h"

// forward declaration
class ComponentTreeDownSampler;
class ComponentTreeConverter;

/**
 * A slice extractor for slices stored in a stack of black-and-white images.
 *
 * Inputs:
 *
 * <table>
 * <tr>
 *   <td>"slices"</td>
 *   <td>(ImageStack)</td>
 *   <td>A stack of black-and-white images to extract the slices from.</td>
 * </tr>
 * <tr>
 *   <td>"force explanation" <b>not yet implemented</b></td>
 *   <td>(bool)</td>
 *   <td>Explain every part of the image, i.e., don't allow free segmentation
 *   hypotheses (where 'free' means it could be picked without violating other
 *   segmentation hypotheses).</td>
 * </tr>
 * </table>
 *
 * Outputs:
 *
 * <table>
 * <tr>
 *   <td>"slices"</td>
 *   <td>(Slices)</td>
 *   <td>All slices extracted from the input image stack.</td>
 * </td>
 * <tr>
 *   <td>"conflict sets"</td>
 *   <td>(ConflictSets)</td>
 *   <td>Conflict sets on the extracted slices. Variable
 *   numbers match slice ids.</td>
 * </tr>
 */
class StackSliceExtractor : public pipeline::ProcessNode {

public:

	/**
	 * Create a new slice extractor for the given section.
	 *
	 * @param section
	 *              The section number that the extracted slices will have.
	 *
	 * @param resX, resY, resZ
	 *              The resolution of the image stack used to extract slices.
	 */
	StackSliceExtractor(unsigned int section, float resX, float resY, float resZ);

private:

	/**
	 * Collects slices from a number of slice sets and establishes conflict sets 
	 * for them.
	 */
	class SliceCollector : public pipeline::SimpleProcessNode<> {

	public:

		SliceCollector();

	private:

		void updateOutputs();

		unsigned int countSlices(const std::vector<Slices>& slices);

		std::vector<Slices> removeDuplicates(const std::vector<Slices>& slices);

		std::vector<Slices> removeDuplicatesPass(const std::vector<Slices>& allSlices);

		void extractSlices(const std::vector<Slices>& slices);

		void extractConstraints(const std::vector<Slices>& slices);

		pipeline::Inputs<Slices> _slices;

		pipeline::Output<Slices> _allSlices;

		pipeline::Output<ConflictSets> _conflictSets;
	};

	void onInputSet(const pipeline::InputSet<ImageStack>& signal);

	// the number of the section this extractor was build for
	unsigned int _section;

	float _resX, _resY, _resZ;

	// the input image stack
	pipeline::Input<ImageStack>         _sliceImageStack;

	// whether to force explanation of every part of the image
	pipeline::Input<bool>               _forceExplanation;

	// extractor to get the images in the input stack
	boost::shared_ptr<ImageExtractor>   _sliceImageExtractor;

	// paramters to use to extract all white connected components
	boost::shared_ptr<ComponentTreeExtractorParameters> _cteParameters;

	// converter from component trees to slices
	boost::shared_ptr<ComponentTreeConverter>   _converter;

	// collector for all slices from all slice images
	boost::shared_ptr<SliceCollector>           _sliceCollector;
};

#endif // SOPNET_STACK_SLICE_EXTRACTOR_H__

