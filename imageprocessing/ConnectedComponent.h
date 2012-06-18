#ifndef IMAGEPROCESSING_CONNECTED_COMPONENT_H__
#define IMAGEPROCESSING_CONNECTED_COMPONENT_H__

#include <boost/shared_ptr.hpp>

#include <imageprocessing/Image.h>
#include <util/point.hpp>
#include <util/rect.hpp>
#include <util/Logger.h>
#include <pipeline/all.h>

class ConnectedComponent : public pipeline::Data {

public:

	typedef std::vector<util::point<unsigned int> > pixel_list_type;
	typedef pixel_list_type::iterator               iterator;
	typedef pixel_list_type::const_iterator         const_iterator;

	ConnectedComponent();

	ConnectedComponent(boost::shared_ptr<Image> source, double value, boost::shared_ptr<pixel_list_type> pixelList, unsigned int begin, unsigned int end);

	double getValue() const;

	void addPixel(const util::point<unsigned int>& pixel);

	void addPixels(const std::vector<util::point<unsigned int> >& pixels);

	const std::pair<const_iterator, const_iterator> getPixels() const;

	const boost::shared_ptr<pixel_list_type> getPixelList() const;

	const unsigned int getSize() const;

	const util::point<double>& getCenter() const;

	const util::rect<double>& getBoundingBox() const;

	bool operator<(const ConnectedComponent& other) const;

private:

	// a list of pixel locations that belong to this component (can be shared
	// between the connected components)
	boost::shared_ptr<pixel_list_type> _pixels;

	// the threshold, at which this connected component was found
	double                                  _value;

	// the min and max x and y values
	util::rect<double>                      _boundingBox;

	// the center of mass of this component
	util::point<double>                     _center;

	// the image, this component was extracted from
	boost::shared_ptr<Image>                _source;

	// the range of the pixels in _pixels that belong to this component (can
	// be all of them, if the pixel lists are not shared)
	iterator _begin;
	iterator _end;
};

#endif // IMAGEPROCESSING_CONNECTED_COMPONENT_H__

