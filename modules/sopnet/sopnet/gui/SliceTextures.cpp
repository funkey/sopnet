#include <imageprocessing/ImageStack.h>
#include <imageprocessing/ConnectedComponent.h>
#include <gui/Texture.h>
#include <util/rect.hpp>
#include <util/point.hpp>
#include "SliceTextures.h"


SliceTextures::~SliceTextures() {

	clear();
}

void
SliceTextures::load(
		const Slice& slice,
		boost::shared_ptr<ImageStack> imageStack) {

	if (_textures.count(slice.getId()))
		return;

	// create image data
	const util::rect<double> bb = slice.getComponent()->getBoundingBox();

	util::point<unsigned int> size(bb.maxX - bb.minX + 2, bb.maxY - bb.minY + 2);
	util::point<unsigned int> offset(bb.minX, bb.minY);

	// rgba data
	std::vector<boost::array<float, 4> > pixels;

	// fill with opaque value

	boost::array<float, 4> opaque;
	opaque[0] = 0.0;
	opaque[1] = 0.0;
	opaque[2] = 0.0;
	opaque[3] = 0.0;
	pixels.resize(size.x*size.y, opaque);

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		unsigned int index = (pixel.x - offset.x) + (pixel.y - offset.y)*size.x;

		float value = 1.0;

		if (imageStack)
			value = (*(*imageStack)[slice.getSection()])(pixel.x, pixel.y);

		pixels[index][0] = value;
		pixels[index][1] = value;
		pixels[index][2] = value;
		pixels[index][3] = 0.5;
	}

	gui::Texture* texture = new gui::Texture(size.x, size.y, GL_RGBA);

	texture->loadData(&pixels[0]);

	_textures[slice.getId()] = texture;
}

void
SliceTextures::clear() {

	unsigned int id;
	gui::Texture* texture;

	foreach (boost::tie(id, texture), _textures)
		if (texture)
			delete texture;

	_textures.clear();
}

gui::Texture*
SliceTextures::get(unsigned int sliceId) {

	if (_textures.count(sliceId))
		return _textures[sliceId];

	BOOST_THROW_EXCEPTION(MissingTexture() << error_message("slice texture does not exist") << STACK_TRACE);
}

