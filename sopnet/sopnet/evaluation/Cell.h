#ifndef SOPNET_EVALUATION_CELL_H__
#define SOPNET_EVALUATION_CELL_H__

#include <set>

/**
 * A cell is a set of connected locations build by intersecting a connected 
 * component of the ground truth with a connected component of the 
 * reconstruction.
 *
 * Cells are annotated with their original reconstruction label, as well as 
 * possible alternative reconstruction labels according to an external tolerance 
 * criterion.
 */
template <typename LabelType>
class Cell {

public:

	/**
	 * A 3D location in the volume.
	 */
	struct Location {

		Location(int x_, int y_, int z_):
			x(x_), y(y_), z(z_) {}

		int x, y, z;

		bool operator<(const Location& other) const {

			if (z < other.z)
				return true;
			else if (z > other.z)
				return false;
			else {

				if (y < other.y)
					return true;
				else if (y > other.y)
					return false;
				else {

					return x < other.x;
				}
			}
		}
	};

	Cell() :
		_min(0, 0, 0),
		_max(0, 0, 0) {}

	/**
	 * Set the original reconstruction label of this cell.
	 */
	void setReconstructionLabel(LabelType k) {

		_label = k;
	}

	/**
	 * Get the original reconstruction label of this cell.
	 */
	LabelType getReconstructionLabel() const {

		return _label;
	}

	/**
	 * Set the ground truth label of this cell.
	 */
	void setGroundTruthLabel(LabelType k) {

		_groundTruthLabel = k;
	}

	/**
	 * Get the ground truth label of this cell.
	 */
	LabelType getGroundTruthLabel() const {

		return _groundTruthLabel;
	}

	/**
	 * Add an alternative label for this cell.
	 */
	void addAlternativeLabel(LabelType k) {

		_alternativeLabels.insert(k);
	}

	/**
	 * Get the current list of alternative labels for this cell.
	 */
	const std::set<LabelType>& getAlternativeLabels() const {

		return _alternativeLabels;
	}

	/**
	 * Add a location to this cell.
	 */
	void add(const Location& l) {

		_content.push_back(l);
	}

	/**
	 * Add a boundary location to this cell.
	 */
	void addBoundary(const Location& l) {

		_boundary.push_back(l);

		_min.x = std::min(_min.x, l.x);
		_min.y = std::min(_min.y, l.y);
		_min.z = std::min(_min.z, l.z);
		_max.x = std::max(_max.x, l.x + 1);
		_max.y = std::max(_max.y, l.y + 1);
		_max.z = std::max(_max.z, l.z + 1);
	}

	/**
	 * Remove a location from this cell. Returns false, if the location was not 
	 * part of this cell.
	 */
	bool remove(const Location& l) {

		iterator i = _content.find(l);

		if (i != _content.end())
			_content.erase(i);
		else
			return false;

		return true;
	}

	bool removeBoundary(const Location& l) {

		iterator i = _boundary.find(l);

		if (i != _boundary.end())
			_boundary.erase(i);
		else
			return false;

		return true;
	}

	/**
	 * Get the number of locations in this cell.
	 */
	unsigned int size() const {

		return _content.size();
	}

	const std::vector<Location>& getBoundary() const {

		return _boundary;
	}

	unsigned int getBoundarySize() const {

		return _boundary.size();
	}

	const Location& getBoundingBoxMin() const {

		return _min;
	}

	const Location& getBoundingBoxMax() const {

		return _max;
	}

	/**
	 * Iterator access to the locations of the cell.
	 */
	typedef typename std::vector<Location>::iterator       iterator;
	typedef typename std::vector<Location>::const_iterator const_iterator;

	iterator begin() { return _content.begin(); }
	iterator end() { return _content.end(); }
	const_iterator begin() const { return _content.begin(); }
	const_iterator end() const { return _content.end(); }

private:

	// the original reconstruction label of this cell
	LabelType _label;

	// the real label of this cell
	LabelType _groundTruthLabel;

	// possible other reconstruction labels, according to the tolerance 
	// criterion
	std::set<LabelType> _alternativeLabels;

	// the volume locations that constitute this cell
	std::vector<Location> _content;

	// the locations that are forming the boundary
	std::vector<Location> _boundary;

	// corners of the bounding box for this cell
	Location _min;
	Location _max;
};

#endif // SOPNET_EVALUATION_CELL_H__

