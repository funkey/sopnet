#ifndef IMAGEPROCESSING_IMAGE_EXTRACTOR_H__
#define IMAGEPROCESSING_IMAGE_EXTRACTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/Image.h>
#include <imageprocessing/ImageStack.h>

// forward declarations
class ImageStack;
class Image;

/**
 * A simple process node that takes an image stack and providese as many image
 * outputs as images have been in the stack. The outputs are created dynamically
 * when the input is set which means that you have to set the input before you
 * ask for any output.
 *
 * Input:
 *
 * <table>
 * <tr>
 *   <td>"stack"</td>
 *   <td>(ImageStack)</td>
 *   <td>The input image stack.</td>
 * </tr>
 * </table>
 *
 * Outputs:
 *
 * <table>
 * <tr>
 *   <td>"image <i>i</i>"</td>
 *   <td>(Image)</td>
 *   <td>As many outputs as images in the input stack.</td>
 * </tr>
 * </table>
 */
class ImageExtractor : public pipeline::SimpleProcessNode<> {

public:

	ImageExtractor();

	/**
	 * Get the number of output images that are currently provided.
	 *
	 * @return The number of output images.
	 */
	unsigned int getNumImages();

private:

	void updateOutputs();

	void onInputSet(const pipeline::InputSet<ImageStack>& signal);

	pipeline::Input<ImageStack> _stack;

	std::vector<pipeline::Output<Image> > _images;
};

#endif // IMAGEPROCESSING_IMAGE_EXTRACTOR_H__

