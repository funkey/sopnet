#include <util/Logger.h>
#include "SplitMerge.h"

logger::LogChannel splitmergelog("splitmergelog", "[SplitMerge] ");

SplitMerge::SplitMerge() :
	_segments(boost::make_shared<Segments>()),
	_painter(boost::make_shared<SplitMergePainter>()),
	_selection(boost::make_shared<std::set<boost::shared_ptr<Slice> > >()),
	_initialSegmentsProcessed(false),
	_drawing(0),
	_lastMousePosition(0, 0) {

	registerInput(_initialSegments, "initial segments");
	registerInput(_section, "section");
	registerOutput(_segments, "segments");
	registerOutput(_painter, "painter");

	setDependency(_section, _painter);

	_initialSegments.registerBackwardCallback(&SplitMerge::onInputSet, this);
	_painter.registerForwardCallback(&SplitMerge::onMouseDown, this);
	_painter.registerForwardCallback(&SplitMerge::onMouseMove, this);
	_painter.registerForwardCallback(&SplitMerge::onMouseUp, this);
	_painter.registerForwardCallback(&SplitMerge::onKeyDown, this);
	_painter->setSelection(_selection);
}

void
SplitMerge::updateOutputs() {

	LOG_ALL(splitmergelog) << "updating outputs" << std::endl;

	if (!_initialSegmentsProcessed) {

		*_segments = *_initialSegments;

		// create end segments for the first section (so that we can select slices 
		// in there)
		std::set<boost::shared_ptr<Slice> > firstSlices;
		unsigned int numNewEnds = 0;
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments(1)) {

			if (segment->getDirection() == Right)
				foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
					firstSlices.insert(slice);
			else
				foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
					firstSlices.insert(slice);
		}
		foreach (boost::shared_ptr<Slice> slice, firstSlices) {

			boost::shared_ptr<EndSegment> newEnd = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice);
			_segments->add(newEnd);

			numNewEnds++;
		}

		LOG_DEBUG(splitmergelog) << "added " << numNewEnds << " new ends" << std::endl;

		_initialSegmentsProcessed = true;
	}

	if (_sliceEditor) {

		_painter->reloadSliceImage();

	} else {

		_painter->setSection(*_section);
	}

	_painter->updateSize();
}

void
SplitMerge::onInputSet(const pipeline::InputSetBase& /*signal*/) {

	_initialSegmentsProcessed = false;

	LOG_DEBUG(splitmergelog) << "initial segments have been set" << std::endl;
}

void
SplitMerge::onKeyDown(gui::KeyDown& signal) {

	if (signal.key == gui::keys::E) {

		if (!_sliceEditor)
			startSliceEditor();
		else
			stopSliceEditor();

		signal.processed = true;

		return;
	}

	if (signal.key == gui::keys::M || signal.key == gui::keys::S) {

		// merge! or split!
		bool doMerge = (signal.key == gui::keys::M);

		std::vector<boost::shared_ptr<Slice> > prevSlices;
		std::vector<boost::shared_ptr<Slice> > currSlices;

		// compare each section with the previous section
		for (unsigned int interval = 0; interval < _segments->getNumInterSectionIntervals(); interval++) {

			foreach (boost::shared_ptr<Slice> slice, *_selection) {

				// slices to the right
				if (slice->getSection() == interval)
					currSlices.push_back(slice);
			}

			LOG_ALL(splitmergelog) << "merging in inter-section interval " << interval << std::endl;
			LOG_ALL(splitmergelog) << "previous slices: " << prevSlices.size() << ", current slices " << currSlices.size() << std::endl;

			// is there something to merge?
			if (prevSlices.size()*currSlices.size() > 0) {

				std::vector<Link> oldLinks;

				removeSegments(prevSlices, Right, interval, oldLinks);
				removeSegments(currSlices, Left, interval, oldLinks);

				if (doMerge) {

					// enumerate all possible links between the slices
					std::vector<Link> links(oldLinks);
					foreach(boost::shared_ptr<Slice> p, prevSlices)
						foreach (boost::shared_ptr<Slice> c, currSlices)
							links.push_back(Link(p, c));

					mergeSlices(interval, links);

				} else { // do a split

					std::set<boost::shared_ptr<Slice> > keptPrevSlices;
					std::set<boost::shared_ptr<Slice> > keptCurrSlices;

					// get all links that should be kept, i.e., not both of 
					// their slices are selected
					std::vector<Link> keptLinks;
					foreach (Link& link, oldLinks) {

						if (std::count(prevSlices.begin(), prevSlices.end(), link.left) == 0) {
							keptPrevSlices.insert(link.left);
							keptLinks.push_back(link);
						}
						
						if (std::count(currSlices.begin(), currSlices.end(), link.right) == 0) {

							keptCurrSlices.insert(link.right);
							keptLinks.push_back(link);
						}
					}

					// re-merge the slices of the links that should be kept
					mergeSlices(interval, keptLinks);

					// end all slices that are not in the kept links
					std::vector<boost::shared_ptr<Slice> > delPrevSlices;
					std::vector<boost::shared_ptr<Slice> > delCurrSlices;

					foreach (boost::shared_ptr<Slice> slice, prevSlices)
						if (keptPrevSlices.count(slice) == 0)
							delPrevSlices.push_back(slice);
					foreach (boost::shared_ptr<Slice> slice, currSlices)
						if (keptCurrSlices.count(slice) == 0)
							delCurrSlices.push_back(slice);

					endSlices(delPrevSlices, Right);
					endSlices(delCurrSlices, Left);
				}
			}

			std::swap(prevSlices, currSlices);
			currSlices.clear();
		}

		setDirty(_segments);
	}
}

void
SplitMerge::removeSegments(const std::vector<boost::shared_ptr<Slice> >& slices, Direction direction, unsigned int interval, std::vector<Link>& oldLinks) {

	// remove all segments in the given interval that are using any of the given 
	// slices in the given direction

	// remember the links that have been represented by these segments -- they 
	// should be maintained

	std::vector<boost::shared_ptr<Segment> > segments = _segments->getSegments(interval);

	LOG_ALL(splitmergelog) << "removing segments in interval " << interval << ", direction " << direction << std::endl;

	foreach (boost::shared_ptr<Segment> segment, segments) {
		LOG_ALL(splitmergelog) << "found segment ";
		foreach (boost::shared_ptr<Slice> segmentSlice, segment->getSourceSlices()) {
			LOG_ALL(splitmergelog) << segmentSlice->getId() << " ";
		}
		LOG_ALL(splitmergelog) << "-> ";
		foreach (boost::shared_ptr<Slice> segmentSlice, segment->getTargetSlices()) {
			LOG_ALL(splitmergelog) << segmentSlice->getId() << " ";
		}
		LOG_ALL(splitmergelog) << segment->getDirection() << std::endl;
	}

	foreach (boost::shared_ptr<Segment> segment, segments) {

		std::vector<boost::shared_ptr<Slice> > leftSlices = segment->getSourceSlices();
		std::vector<boost::shared_ptr<Slice> > rightSlices = segment->getTargetSlices();

		if (segment->getDirection() == Left)
			std::swap(leftSlices, rightSlices);

		bool removed = false;

		// for each given slices
		foreach (boost::shared_ptr<Slice> slice, slices) {

			// does the current segment use it on the site we are interested in?
			foreach (boost::shared_ptr<Slice> segmentSlice, (direction == Right ? leftSlices : rightSlices)) {
				if (segmentSlice->getId() == slice->getId()) {

					_segments->remove(segment);

					LOG_DEBUG(splitmergelog) << "removed a segment using slice " << slice->getId() << " in interval " << interval << std::endl;

					foreach (boost::shared_ptr<Slice> l, leftSlices)
						foreach (boost::shared_ptr<Slice> r, rightSlices) {

							LOG_ALL(splitmergelog) << "remembering link between " << l->getId() << " and " << r->getId() << std::endl;
							oldLinks.push_back(Link(l, r));
						}

					removed = true;
					break;
				}
			}

			if (removed)
				break;
		}
	}
}

void
SplitMerge::mergeSlices(unsigned int interval, std::vector<Link>& links) {

	// get all slices that are involved
	std::set<boost::shared_ptr<Slice> > slices;
	foreach (Link& link, links) {
		slices.insert(link.left);
		slices.insert(link.right);
	}
	unsigned int numSlices = slices.size();

	LOG_ALL(splitmergelog) << "merging " << numSlices << " slices" << std::endl;

	std::sort(links.begin(), links.end());

	std::map<boost::shared_ptr<Slice>, std::set<boost::shared_ptr<Slice> > > partners;

	// process links from closest to furthest to identify partner sets, stop as 
	// soon as all slices are in one set
	foreach (Link& link, links) {

		// if we found partners for each slice
		if (partners.size() == numSlices)
			break;

		boost::shared_ptr<Slice> p = link.left;
		boost::shared_ptr<Slice> c = link.right;

		// partners to themselves
		partners[p].insert(p);
		partners[c].insert(c);

		// get all partners of c and add them to the partners of p
		foreach (boost::shared_ptr<Slice> s, partners[c])
			partners[p].insert(s);

		// p is now the only one who knows all its partners

		// the partners of p might not know about c or any of its partners

		// for each partner of p, add all partners of p to the partner list
		foreach (boost::shared_ptr<Slice> pp, partners[p]) {

			foreach (boost::shared_ptr<Slice> ppp, partners[p])
				partners[pp].insert(ppp);
		}
	}

	std::set<boost::shared_ptr<Slice> > linkedSlices;

	boost::shared_ptr<Slice>            s;
	std::set<boost::shared_ptr<Slice> > ps;
	foreach (boost::tie(s, ps), partners) {

		// all slices have been linked by segments
		if (linkedSlices.size() == numSlices)
			break;

		// this slice has been linked already
		if (linkedSlices.count(s))
			continue;

		// now we connect s to all its partners ps with segments

		// remember all slices that have been handled so far
		foreach (boost::shared_ptr<Slice> p, ps)
			linkedSlices.insert(p);

		// if there is only one partner, it's the slice itself, so this has to 
		// become an end
		if (ps.size() == 0) {

			boost::shared_ptr<Slice> slice = *ps.begin();
			if (slice->getSection() == interval)
				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice));
			else
				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, slice));

			continue;
		}

		// for each pair of partners, introduce a continuation segment
		typename std::set<boost::shared_ptr<Slice> >::iterator i, j;
		for (i = ps.begin(); i != ps.end(); i++)
			for (j = i; j != ps.end(); j++) {

				// if both are in the same section
				if ((*i)->getSection() == (*j)->getSection())
					continue;

				boost::shared_ptr<Slice> left;
				boost::shared_ptr<Slice> right;

				if ((*i)->getSection() < (*j)->getSection()) {

					left  = *i;
					right = *j;

				} else {

					left  = *j;
					right = *i;
				}

				boost::shared_ptr<ContinuationSegment> continuation = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, left, right);

				_segments->add(continuation);

				LOG_DEBUG(splitmergelog) << "added a continuation between slice " << left->getId() << " and " << right->getId() << std::endl;
			}
	}
}

void
SplitMerge::endSlices(std::vector<boost::shared_ptr<Slice> >& slices, Direction direction) {

	foreach (boost::shared_ptr<Slice> slice, slices)
		endSlice(slice, direction);
}

void
SplitMerge::endSlice(boost::shared_ptr<Slice> slice, Direction direction) {

	// end a slices to right or left, if no other segment is using it in this 
	// direction

	bool skip = false;

	unsigned int interval = slice->getSection() + (direction == Right ? 1 : 0);

	// try to find a segment that uses this slice
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments(interval)) {
		foreach (boost::shared_ptr<Slice> s, (segment->getDirection() == direction ? segment->getSourceSlices() : segment->getTargetSlices())) {

			if (s->getId() == slice->getId()) {
				skip = true;
				break;
			}
		}

		if (skip)
			break;
	}

	if (skip)
		return;

	LOG_ALL(splitmergelog) << "adding end for slice " << slice->getId() << " to " << (direction == Left ? "left" : "right") << std::endl;
	_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice));
}

void
SplitMerge::onMouseDown(const gui::MouseDown& signal) {

	if (signal.modifiers & gui::keys::ControlDown)
		return;

	LOG_ALL(splitmergelog) << "mouse down" << std::endl;

	if (signal.button == gui::buttons::Left) {

		if (_sliceEditor) {

			LOG_ALL(splitmergelog) << "drawing at " << signal.position << std::endl;

			_sliceEditor->draw(signal.position, 3, true /* foreground */);
			_drawing = 1;
			setDirty(_painter);

			return;
		}

		std::vector<boost::shared_ptr<Slice> > closestSlices = getCurrentSlices(signal.position);

		if (closestSlices.size() == 0) {

			LOG_ALL(splitmergelog) << "found no slices in this section" << std::endl;
			return;
		}

		// find the closest of all slices
		boost::shared_ptr<Slice> closestSlice;
		double distance = 0;
		for (unsigned int i = 0; i < closestSlices.size(); i++) {

			util::point<double> center = closestSlices[i]->getComponent()->getCenter();
			double d = pow(center.x - signal.position.x, 2) + pow(center.y - signal.position.y,2);

			if (!closestSlice || distance > d) {

				closestSlice = closestSlices[i];
				distance = d;
			}
		}

		LOG_ALL(splitmergelog) << "slice " << closestSlice->getId() << " is closest to click" << std::endl;

		if (_selection->count(closestSlice)) {

			LOG_ALL(splitmergelog) << "remove it from selection" << std::endl;
			_selection->erase(_selection->find(closestSlice));

		} else {

			LOG_ALL(splitmergelog) << "add it to selection" << std::endl;
			_selection->insert(closestSlice);
		}

		setDirty(_painter);
	}

	if (signal.button == gui::buttons::Right) {

		if (_sliceEditor) {

			_sliceEditor->draw(signal.position, 3, false /* background */);
			_drawing = 2;
			setDirty(_painter);

			return;
		}

		_selection->clear();
		setDirty(_painter);
	}
}

void
SplitMerge::onMouseMove(const gui::MouseMove& signal) {

	_lastMousePosition = signal.position;

	if (!_sliceEditor || !_drawing)
		return;

	_sliceEditor->draw(signal.position, 3, (_drawing == 1 ? true /* foreground */ : false /* background */));
	setDirty(_painter);
}

void
SplitMerge::onMouseUp(const gui::MouseUp& /*signal*/) {

	_drawing = 0;
}

std::vector<boost::shared_ptr<Slice> >
SplitMerge::getCurrentSlices(const util::point<double>& position) {

	// find closest segments in intersection interval to the left and right
	std::vector<boost::shared_ptr<Segment> > closestSegments;
	for (int section = *_section; section <= *_section + 1; section++) {

		std::vector<boost::shared_ptr<EndSegment> >          ends          = _segments->findEnds(position, *_section, 1000000);
		std::vector<boost::shared_ptr<ContinuationSegment> > continuations = _segments->findContinuations(position, *_section, 1000000);

		std::copy(ends.begin(),          ends.end(),          std::back_inserter(closestSegments));
		std::copy(continuations.begin(), continuations.end(), std::back_inserter(closestSegments));
	}

	// get all the slices that are in the current section
	std::vector<boost::shared_ptr<Slice> > closestSlices;

	foreach (boost::shared_ptr<Segment> segment, closestSegments) {

		foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
			if (slice->getSection() == (unsigned int)*_section)
				closestSlices.push_back(slice);
		foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
			if (slice->getSection() == (unsigned int)*_section)
				closestSlices.push_back(slice);
	}

	return closestSlices;
}

void
SplitMerge::startSliceEditor() {

	LOG_DEBUG(splitmergelog) << "starting slice editor" << std::endl;

	// get all slices in current section
	std::vector<boost::shared_ptr<Slice> > currentSlices = getCurrentSlices(_lastMousePosition);

	// get their bounding box
	util::rect<int> bb(0, 0, 0, 0);
	foreach (boost::shared_ptr<Slice> slice, currentSlices)
		if (bb.isZero())
			bb = slice->getComponent()->getBoundingBox();
		else
			bb.fit(slice->getComponent()->getBoundingBox());

	// create slice editor
	_sliceEditor = boost::make_shared<SliceEditor>(currentSlices, *_section, bb);

	// show slice image
	_painter->setSliceImage(_sliceEditor->getSliceImage(), bb.upperLeft());

	setDirty(_painter);
}

void
SplitMerge::stopSliceEditor() {

	LOG_DEBUG(splitmergelog) << "stopping slice editor" << std::endl;

	SliceEdits edits = _sliceEditor->finish();

	// destruct slice editor
	_sliceEditor.reset();

	// unset slice image
	_painter->unsetSliceImage();

	// process edits
	foreach (const SliceReplacements& replacements, edits.getReplacements())
		processSliceReplacements(replacements);

	setDirty(_painter);
	setDirty(_segments);
}

void
SplitMerge::processSliceReplacements(const SliceReplacements& replacements) {

	const std::vector<boost::shared_ptr<Slice> >& oldSlices = replacements.getOldSlices();
	const std::vector<boost::shared_ptr<Slice> >& newSlices = replacements.getNewSlices();

	if (oldSlices.size() + newSlices.size() == 0)
		return;

	unsigned int section = (oldSlices.size() > 0 ? oldSlices[0]->getSection() : newSlices[0]->getSection());

	std::vector<Link> oldPrevLinks;
	std::vector<Link> oldNextLinks;

	// remove all segments to old slices
	removeSegments(oldSlices, Right, section,     oldPrevLinks);
	removeSegments(oldSlices, Left , section,     oldPrevLinks);
	removeSegments(oldSlices, Right, section + 1, oldNextLinks);
	removeSegments(oldSlices, Left,  section + 1, oldNextLinks);

	LOG_ALL(splitmergelog) << "old slices had " << oldPrevLinks.size() << " links to previous section" << std::endl;
	LOG_ALL(splitmergelog) << "old slices had " << oldNextLinks.size() << " links to next section" << std::endl;

	// 1:n or n:1 replacement
	if (oldSlices.size() == 1 || newSlices.size() == 1) {

		LOG_DEBUG(splitmergelog) << "processing 1:n or n:1 slice replacement" << std::endl;

		// old slices got deleted
		if (newSlices.size() == 0) {

			foreach (const Link& link, oldPrevLinks)
				endSlice(link.left, Right);

			foreach (const Link& link, oldNextLinks)
				endSlice(link.right, Left);

			return;
		}

		// create new segments for new slices
		foreach (boost::shared_ptr<Slice> newSlice, newSlices) {

			if (oldPrevLinks.size() > 0) {

				foreach (const Link& link, oldPrevLinks) {

					LOG_ALL(splitmergelog) << "linking new slice with previous slice " << link.left->getId() << std::endl;

					boost::shared_ptr<ContinuationSegment> continuation =
							boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, link.left, newSlice);
					_segments->add(continuation);
				}

			} else {

				LOG_ALL(splitmergelog) << "new slice ends to previous section" << std::endl;

				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, newSlice));
			}

			if (oldNextLinks.size() > 0) {

				foreach (const Link& link, oldNextLinks) {

					LOG_ALL(splitmergelog) << "linking new slice with next slice " << link.right->getId() << std::endl;

					boost::shared_ptr<ContinuationSegment> continuation =
							boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, newSlice, link.right);
					_segments->add(continuation);
				}

			} else {

				LOG_ALL(splitmergelog) << "new slice ends to next section" << std::endl;

				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, newSlice));
			}
		}

		return;
	}

	// n:m replacement, just create new disconnected slices
	
	foreach (const Link& link, oldPrevLinks)
		endSlice(link.left, Right);

	foreach (const Link& link, oldNextLinks)
		endSlice(link.right, Left);

	// create new segments for new slices
	foreach (boost::shared_ptr<Slice> newSlice, newSlices) {

		LOG_ALL(splitmergelog) << "new slice ends to previous section" << std::endl;

		_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, newSlice));

		LOG_ALL(splitmergelog) << "new slice ends to next section" << std::endl;

		_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, newSlice));
	}
}
