#ifndef IMAGEPROCESSING_IMAGE_STACK_H__
#define IMAGEPROCESSING_IMAGE_STACK_H__

#include <pipeline/all.h>
#include <imageprocessing/Image.h>

class ImageStack : public pipeline::Data {

	// Image objects are shared between ImageStack
	typedef std::vector<boost::shared_ptr<Image> > sections_type;

public:

	typedef sections_type::iterator         iterator;

	typedef sections_type::const_iterator   const_iterator;

	/**
	 * Remove all sections.
	 */
	void clear();

	/**
	 * Add a single section to this set of sections.
	 */
	void add(boost::shared_ptr<Image> section);

	/**
	 * Add a set of sections to this set of sections.
	 */
	void addAll(boost::shared_ptr<ImageStack> stack);

	const const_iterator begin() const { return _sections.begin(); }

	iterator begin() { return _sections.begin(); }

	const const_iterator end() const { return _sections.end(); }

	iterator end() { return _sections.end(); }

	boost::shared_ptr<Image> operator[](unsigned int i) { return _sections[i]; }

	unsigned int size() { return _sections.size(); }

private:

	sections_type _sections;

};

#endif // IMAGEPROCESSING_IMAGE_STACK_H__

