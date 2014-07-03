#ifndef SOPNET_SKELETONS_SPHERES_H__
#define SOPNET_SKELETONS_SPHERES_H__

#include <vector>
#include <pipeline/Data.h>
#include "Sphere.h"

class Spheres : public pipeline::Data {

	typedef std::vector<Sphere> spheres_type;

public:

	typedef spheres_type::iterator       iterator;
	typedef spheres_type::const_iterator const_iterator;

	void add(const Sphere& sphere) { _spheres.push_back(sphere); }

	iterator       begin()       { return _spheres.begin(); }
	const_iterator begin() const { return _spheres.begin(); }
	iterator       end()         { return _spheres.end(); }
	const_iterator end()   const { return _spheres.end(); }

private:

	spheres_type _spheres;
};

#endif // SOPNET_SKELETONS_SPHERES_H__

