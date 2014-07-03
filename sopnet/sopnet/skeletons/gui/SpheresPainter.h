#ifndef SOPNET_SKELETONS_GUI_SPHERES_PAINTER_H__
#define SOPNET_SKELETONS_GUI_SPHERES_PAINTER_H__

#include <gui/RecordablePainter.h>
#include <sopnet/skeletons/Spheres.h>

class SpheresPainter : public gui::RecordablePainter {

public:

	/**
	 * Set the spheres to render.
	 */
	void setSpheres(boost::shared_ptr<Spheres> spheres);

private:

	void updateRecording();

	boost::shared_ptr<Spheres> _spheres;
	util::rect<double>         _size;
};

#endif // SOPNET_SKELETONS_GUI_SPHERES_PAINTER_H__

