#include <boost/lexical_cast.hpp>

#include "ComponentTreeHdf5Writer.h"

logger::LogChannel componenttreehdf5writerlog("componenttreehdf5writerlog", "[ComponentTreeHdf5Writer] ");

ComponentTreeHdf5Writer::ComponentTreeHdf5Writer(const H5::Group& group) :
	_group(group) {

	registerInput(_componentTree, "component tree");
}

void
ComponentTreeHdf5Writer::write() {

	updateInputs();

	writeComponentTree();
}

void
ComponentTreeHdf5Writer::updateOutputs() {

	writeComponentTree();
}

void
ComponentTreeHdf5Writer::writeComponentTree() {

	unsigned int numComponents = _componentTree->size();

	WriteVisitor visitor(numComponents);

	_componentTree->visit(visitor);

	visitor.save(_group);
}

ComponentTreeHdf5Writer::WriteVisitor::WriteVisitor(unsigned int numComponents) :
	_pixelListIds(numComponents),
	_begins(numComponents),
	_ends(numComponents),
	_minXs(numComponents),
	_maxXs(numComponents),
	_minYs(numComponents),
	_maxYs(numComponents),
	_values(numComponents),
	_numPixelLists(0),
	_numComponents(0) {}

void
ComponentTreeHdf5Writer::WriteVisitor::visitNode(boost::shared_ptr<ComponentTree::Node> node) {

	boost::shared_ptr<ConnectedComponent> component = node->getComponent();
	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList = component->getPixelList();

	if (_savedPixelLists.count(pixelList) == 0) {

		_savedPixelLists[pixelList] = _numPixelLists;
		_numPixelLists++;
	}

	unsigned int pixelListId = _savedPixelLists[pixelList];

	unsigned int begin = component->getPixels().first - pixelList->begin();
	unsigned int end   = component->getPixels().second - pixelList->begin();

	_pixelListIds[_numComponents] = pixelListId;
	_begins[_numComponents]       = begin;
	_ends[_numComponents]         = end;
	_minXs[_numComponents]        = component->getBoundingBox().minX;
	_maxXs[_numComponents]        = component->getBoundingBox().maxX;
	_minYs[_numComponents]        = component->getBoundingBox().minY;
	_maxYs[_numComponents]        = component->getBoundingBox().maxY;
	_values[_numComponents]       = component->getValue();

	_numComponents++;
}

void
ComponentTreeHdf5Writer::WriteVisitor::save(H5::Group& group) {

	std::vector<unsigned int> dims;
	dims.push_back(_numComponents);

	// write the per-component data

	hdf5::write(group, "pixel_list_ids", _pixelListIds, dims);
	hdf5::write(group, "begin_indices",  _begins,       dims);
	hdf5::write(group, "end_indices",    _ends,         dims);
	hdf5::write(group, "min_x",          _minXs,        dims);
	hdf5::write(group, "max_x",          _maxXs,        dims);
	hdf5::write(group, "min_y",          _minYs,        dims);
	hdf5::write(group, "max_y",          _maxYs,        dims);
	hdf5::write(group, "values",         _values,       dims);

	// write the shared pixel lists

	typedef util::point<unsigned int> pixel_type;

	H5::CompType pixelType(sizeof(pixel_type));
	pixelType.insertMember("x", HOFFSET(pixel_type, x), H5::PredType::NATIVE_UINT);
	pixelType.insertMember("y", HOFFSET(pixel_type, y), H5::PredType::NATIVE_UINT);

	typedef std::pair<const boost::shared_ptr<ConnectedComponent::pixel_list_type>, unsigned int> list_id_pair_t;
	foreach (list_id_pair_t& i, _savedPixelLists) {

		boost::shared_ptr<ConnectedComponent::pixel_list_type> list = i.first;
		unsigned int id = i.second;

		dims.clear();
		dims.push_back(list->size());

		hdf5::write(group, "pixel_list_" + boost::lexical_cast<std::string>(id), *list, dims, pixelType);
	}
}
