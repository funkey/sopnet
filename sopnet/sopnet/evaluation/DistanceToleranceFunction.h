#ifndef SOPNET_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__
#define SOPNET_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__

#include "LocalToleranceFunction.h"

class DistanceToleranceFunction : public LocalToleranceFunction {

public:

	DistanceToleranceFunction(float distanceThreshold);

	void extractCells(
			unsigned int numCells,
			const vigra::MultiArray<3, unsigned int>& cellLabels,
			const ImageStack& recLabels,
			const ImageStack& gtLabels);

private:

	struct BoundaryLocations {

		std::vector<cell_t::Location> locations;

		int resX, resY, resZ;

		// Must return the number of data points
		inline size_t kdtree_get_point_count() const { return locations.size(); }

		// Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
		inline unsigned int kdtree_distance(const int *p1, const size_t idx_p2, size_t /*size*/) const {

			const int d0 = (p1[0] - locations[idx_p2].x)*resX;
			const int d1 = (p1[1] - locations[idx_p2].y)*resY;
			const int d2 = (p1[2] - locations[idx_p2].z)*resZ;
			return d0*d0+d1*d1+d2*d2;
		}

		// Returns the dim'th component of the idx'th point in the class:
		// Since this is inlined and the "dim" argument is typically an immediate value, the
		//  "if/else's" are actually solved at compile time.
		inline int kdtree_get_pt(const size_t idx, int dim) const
		{
			if (dim==0) return locations[idx].x;
			else if (dim==1) return locations[idx].y;
			else return locations[idx].z;
		}

		// Optional bounding-box computation: return false to default to a standard bbox computation loop.
		//   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
		//   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
		template <class BBOX>
		bool kdtree_get_bbox(BBOX &/*bb*/) const { return false; }

	};

	// functor to test coverage of locations from a center point and radius
	struct covered_from {

		covered_from(int x_, int y_, int z_) :
			x(x_), y(y_), z(z_) {}

		bool operator()(const cell_t::Location& l) {

			return
					((x-l.x)*(x-l.x)*resX2 +
					 (y-l.y)*(y-l.y)*resY2 +
					 (z-l.z)*(z-l.z)*resZ2 <= r2);
		}

		// center
		int x, y, z;

		// resolution in x, y, z, and max radius
		static int resX2, resY2, resZ2, r2;
	};

	// find alternative cell labels
	void enumerateCellLabels(const ImageStack& recLabels, const vigra::MultiArray<3, unsigned int>& cellLabels);

	// create a b/w image of reconstruction label changes
	void createBoundaryMap(const ImageStack& recLabels);

	// create a distance2 image of boundary distances
	void createBoundaryDistanceMap();

	// find all offset locations for the given distance threshold
	std::vector<cell_t::Location> createNeighborhood();

	// search for all relabeling alternatives for the given cell and 
	// neighborhood
	std::set<float> getAlternativeLabels(
			const cell_t& cell,
			const std::vector<cell_t::Location>& neighborhood,
			const ImageStack& recLabels);

	// test, whether a voxel is surrounded by at least one other voxel with a 
	// different label
	bool isBoundaryVoxel(int x, int y, int z, const ImageStack& stack);

	// the distance threshold in nm
	float _maxDistanceThreshold;

	// the size of one voxel
	float _resolutionX;
	float _resolutionY;
	float _resolutionZ;

	// the distance threshold in pixels for each direction
	int _maxDistanceThresholdX;
	int _maxDistanceThresholdY;
	int _maxDistanceThresholdZ;

	// the extends of the ground truth and reconstruction
	unsigned int _width, _height, _depth;

	vigra::MultiArray<3, short> _boundaryMap;
	vigra::MultiArray<3, float> _boundaryDistance2;

	std::vector<unsigned int> _relabelCandidates;
};

#endif // SOPNET_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__

