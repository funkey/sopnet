#ifndef CELLTRACKER_CELL_H__
#define CELLTRACKER_CELL_H__

#include <boost/shared_ptr.hpp>

// forward declaration
class ConnectedComponent;

class Slice {

public:

	Slice(
			unsigned int id,
			unsigned int section,
			boost::shared_ptr<ConnectedComponent> component);

	unsigned int getId() const;

	unsigned int getSection() const;

	boost::shared_ptr<ConnectedComponent> getComponent() const;

private:

	unsigned int _id;

	unsigned int _section;

	boost::shared_ptr<ConnectedComponent> _component;
};

#endif // CELLTRACKER_CELL_H__

