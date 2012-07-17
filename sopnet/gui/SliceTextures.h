#ifndef SOPNET_GUI_SLICE_TEXTURES_H__
#define SOPNET_GUI_SLICE_TEXTURES_H__

#include <map>

#include <boost/shared_ptr.hpp>

#include <gui/Texture.h>
#include <sopnet/slices/Slice.h>

// forward declaration
class ImageStack;

class SliceTextures {

public:

	~SliceTextures();

	/**
	 * Load the texture for a slice. Optionally, an image can be given to texture the slice.
	 */
	void load(const Slice& slice, boost::shared_ptr<ImageStack> image = boost::shared_ptr<ImageStack>());

	/**
	 * Delete all textures.
	 */
	void clear();

	/**
	 * Get the texture for the given slice id.
	 */
	gui::Texture* get(unsigned int sliceId);

private:

	std::map<unsigned int, gui::Texture*> _textures;
};

#endif // SOPNET_GUI_SLICE_TEXTURES_H__

