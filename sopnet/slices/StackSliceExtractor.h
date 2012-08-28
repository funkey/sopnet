#ifndef SOPNET_STACK_SLICE_EXTRACTOR_H__
#define SOPNET_STACK_SLICE_EXTRACTOR_H__

#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/MserParameters.h>
#include "Slices.h"

// forward declaration
class ComponentTreeDownSampler;
class ComponentTreeConverter;
class Mser;
class MserParameters;

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
 *   <td>"linear constraints"</td>
 *   <td>(LinearConstraints)</td>
 *   <td>Linear consistency constraints on the extracted slices. Variable
 *   numbers match slice ids.</td>
 * </tr>
 */
class StackSliceExtractor : public pipeline::ProcessNode {

public:

	StackSliceExtractor(unsigned int section);

private:

	/**
	 * Collects slices from a number of slice sets and establishes linear
	 * consistency constraints for them.
	 */
	class SliceCollector : public pipeline::SimpleProcessNode {

	public:

		SliceCollector();

	private:

		void updateOutputs();

		pipeline::Inputs<Slices> _slices;

		pipeline::Output<Slices> _allSlices;

		pipeline::Output<LinearConstraints> _linearConstraints;
	};

	/**
	 * Functor to access slice coordinates in the kd-tree.
	 */
	struct SliceCoordinates {

		double operator()(boost::shared_ptr<Slice> slice, size_t i) {

			const util::point<double>& center = slice->getComponent()->getCenter();

			if (i == 0)
				return center.x;

			return center.y;
		}
	};

	void onInputSet(const pipeline::InputSet<ImageStack>& signal);

	void extractSlices();

	// the number of the section this extractor was build for
	unsigned int _section;

	// the input image stack
	pipeline::Input<ImageStack>         _sliceImageStack;

	// whether to force explanation of every part of the image
	pipeline::Input<bool>               _forceExplanation;

	// all slices extracted from the slice image stack
	pipeline::Output<Slices>            _slices;

	// linear consistency constraints on the slices
	pipeline::Output<LinearConstraints> _linearConstraints;

	// extractor to get the images in the input stack
	boost::shared_ptr<ImageExtractor>           _sliceImageExtractor;

	// one mser per slice image
	std::vector<boost::shared_ptr<Mser> >       _msers;

	// mser paramters to use to extract all white connected components
	boost::shared_ptr<MserParameters>           _mserParameters;

	// converter from component trees to slices
	boost::shared_ptr<ComponentTreeConverter>   _converter;

	// collector for all slices from all slice images
	boost::shared_ptr<SliceCollector>           _sliceCollector;
};

#endif // SOPNET_STACK_SLICE_EXTRACTOR_H__

