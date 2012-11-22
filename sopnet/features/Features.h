#ifndef SOPNET_FEATURES_H__
#define SOPNET_FEATURES_H__

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <pipeline/all.h>

class Features : public pipeline::Data {

	typedef std::vector<std::vector<double> > features_type;

	typedef std::map<unsigned int, unsigned int> segment_ids_map;

public:

	typedef features_type::iterator iterator;

	typedef features_type::const_iterator const_iterator;

	void addName(const std::string& name);

	const std::vector<std::string>& getNames() const;

	void clear();

	void add(unsigned int segmentId, const std::vector<double>& features);

	const std::vector<double>& get(unsigned int segmentId);

	unsigned int size();

	iterator begin();

	iterator end();

	const_iterator begin() const;

	const_iterator end() const;

	std::vector<double>& operator[](unsigned int i);

	const std::vector<double>& operator[](unsigned int i) const;

	void setSegmentIdsMap(const segment_ids_map& map);

	const segment_ids_map& getSegmentsIdsMap() const;

	static double NoFeatureValue;

private:

	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& archive, const unsigned int version) {

		archive & _features;
		archive & _featureNames;
		archive & _segmentIdsMap;
	}

	features_type            _features;

	std::vector<std::string> _featureNames;

	// a map from segment ids to the corresponding feature
	segment_ids_map          _segmentIdsMap;
};

std::ostream&
operator<<(std::ostream& out, const Features& features);

#endif // SOPNET_FEATURES_H__

