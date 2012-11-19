#ifndef IMAGEPROCESSING_IO_COMPONENT_TREE_HDF5_WRITER_H__
#define IMAGEPROCESSING_IO_COMPONENT_TREE_HDF5_WRITER_H__

#include <pipeline/all.h>
#include <util/hdf5.h>
#include <imageprocessing/ComponentTree.h>

class ComponentTreeHdf5Writer : public pipeline::SimpleProcessNode<> {

public:

	ComponentTreeHdf5Writer(const H5::Group& group);

	void write();

private:

	class WriteVisitor : public ComponentTree::Visitor {

	public:

		WriteVisitor(unsigned int numComponents);

		void visitNode(boost::shared_ptr<ComponentTree::Node> node);
		void visitEdge(boost::shared_ptr<ComponentTree::Node>, boost::shared_ptr<ComponentTree::Node>) {};

		void save(H5::Group& group);

	private:

		// a map of all pixel lists, that have been stored in the file already
		std::map<boost::shared_ptr<ConnectedComponent::pixel_list_type>, unsigned int> _savedPixelLists;

		// the data to store (one entry per connected component)
		std::vector<unsigned int> _pixelListIds;
		std::vector<unsigned int> _begins;
		std::vector<unsigned int> _ends;
		std::vector<unsigned int> _minXs;
		std::vector<unsigned int> _maxXs;
		std::vector<unsigned int> _minYs;
		std::vector<unsigned int> _maxYs;
		std::vector<double>       _values;

		// the index of the current pixel list
		unsigned int _numPixelLists;

		// the index of the current connected component
		unsigned int _numComponents;
	};

	void updateOutputs();

	void writeComponentTree();

	pipeline::Input<ComponentTree> _componentTree;

	H5::Group _group;
};

#endif // IMAGEPROCESSING_IO_COMPONENT_TREE_HDF5_WRITER_H__

