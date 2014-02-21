#ifndef SOPNET_SEGMENTS_SPLIT_MERGE_H__
#define SOPNET_SEGMENTS_SPLIT_MERGE_H__

#include <gui/MouseSignals.h>
#include <gui/KeySignals.h>
#include <pipeline/SimpleProcessNode.h>

#include <sopnet/gui/SplitMergePainter.h>
#include <sopnet/slices/SliceEditor.h>

class SplitMerge : public pipeline::SimpleProcessNode<> {

public:

	SplitMerge();

private:

	struct Link {

		Link(boost::shared_ptr<Slice> l, boost::shared_ptr<Slice> r) : left(l), right(r) {}

		bool operator<(const Link& other) const {

			return distance() < other.distance();
		}

		double distance() const {

			return
					pow(left->getComponent()->getCenter().x - right->getComponent()->getCenter().x, 2) +
					pow(left->getComponent()->getCenter().y - right->getComponent()->getCenter().y, 2);
		}

		boost::shared_ptr<Slice> left;
		boost::shared_ptr<Slice> right;
	};

	void updateOutputs();

	void onInputSet(const pipeline::InputSetBase& signal);

	void onKeyDown(gui::KeyDown& signal);
	void onMouseDown(const gui::MouseDown& signal);
	void onMouseMove(const gui::MouseMove& signal);
	void onMouseUp(const gui::MouseUp& signal);

	std::vector<boost::shared_ptr<Slice> > getCurrentSlices(const util::point<double>& position = util::point<double>(0, 0));

	void removeSegments(std::vector<boost::shared_ptr<Slice> >& slices, Direction direction, unsigned int interval, std::vector<Link>& oldLinks);
	void mergeSlices(unsigned int interval, std::vector<Link>& oldLinks);
	void endSlices(std::vector<boost::shared_ptr<Slice> >& prevSlices, std::vector<boost::shared_ptr<Slice> >& currSlices);

	void startSliceEditor();
	void stopSliceEditor();

	pipeline::Input<Segments>           _initialSegments;
	pipeline::Input<int>                _section;
	pipeline::Output<Segments>          _segments;
	pipeline::Output<SplitMergePainter> _painter;

	boost::shared_ptr<std::set<boost::shared_ptr<Slice> > > _selection;

	bool _initialSegmentsProcessed;

	boost::shared_ptr<SliceEditor> _sliceEditor;
	int _drawing;
};

#endif // SOPNET_SEGMENTS_SPLIT_MERGE_H__

