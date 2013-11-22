#ifndef SOPNET_GUI_FEATURES_PAINTER_H__
#define SOPNET_GUI_FEATURES_PAINTER_H__

#include <iomanip>
#include <vector>

#include <boost/make_shared.hpp>

#include <gui/Painter.h>
#include <gui/TextPainter.h>

class FeaturesPainter : public gui::Painter {

public:

	FeaturesPainter(unsigned int numFeatures = 0);

	void setCosts(double costs);

	void setFeatures(const std::vector<double>& features);

	void setFeatureNames(const std::vector<std::string>& names);

	bool draw(const util::rect<double>& roi, const util::point<double>& resolution);

private:

	template <typename T>
	boost::shared_ptr<gui::TextPainter> createTextPainter(T value) {

		std::stringstream ss;

		ss << std::setprecision(2) << std::fixed << value;

		boost::shared_ptr<gui::TextPainter> painter = boost::make_shared<gui::TextPainter>(ss.str());
		painter->setTextSize(10.0);

		return painter;
	}

	void updateTextPainters();

	double                   _costs;
	std::vector<double>      _features;
	std::vector<std::string> _featureNames;

	// the size of one _cell
	float _cellX, _cellY;

	// text painters and their position
	std::vector<std::pair<boost::shared_ptr<gui::TextPainter>, util::point<int> > > _textPainters;

	// do the text painters need to be updated?
	bool _textPaintersDirty;
};

#endif // SOPNET_GUI_FEATURES_PAINTER_H__

