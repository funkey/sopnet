#include <boost/make_shared.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <iostream>
#include "Slice.h"

Slice::Slice(
		unsigned int id,
		unsigned int section,
		boost::shared_ptr<ConnectedComponent> component) :
	_id(id),
	_section(section),
	_component(component) {}

unsigned int
Slice::getId() const {

	return _id;
}

unsigned int
Slice::getSection() const {

	return _section;
}

boost::shared_ptr<ConnectedComponent>
Slice::getComponent() const {

	return _component;
}

void
Slice::intersect(const Slice& other) {

	_component = boost::make_shared<ConnectedComponent>(getComponent()->intersect(*other.getComponent()));

	setBoundingBoxDirty();
	setHashDirty();
}

void
Slice::translate(const util::point<int>& pt)
{
	_component = boost::make_shared<ConnectedComponent>(getComponent()->translate(pt));

	setBoundingBoxDirty();
	setHashDirty();
}

bool
Slice::operator==(const Slice& other) const
{
	return getSection() == other.getSection() && (*getComponent()) == (*other.getComponent());
}

BoundingBox
Slice::computeBoundingBox() const {

	const util::rect<double>& componentBoundingBox = getComponent()->getBoundingBox();

	return BoundingBox(
			componentBoundingBox.minX*getResolutionX(),
			componentBoundingBox.minY*getResolutionY(),
			 _section                *getResolutionZ(),
			componentBoundingBox.maxX*getResolutionX(),
			componentBoundingBox.maxY*getResolutionY(),
			(_section + 1)           *getResolutionZ());
}
