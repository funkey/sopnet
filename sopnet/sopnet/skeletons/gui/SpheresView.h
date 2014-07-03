#ifndef SOPNET_SKELETONS_GUI_SPHERES_VIEW_H__
#define SOPNET_SKELETONS_GUI_SPHERES_VIEW_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/skeletons/Spheres.h>
#include "SpheresPainter.h"

class SpheresView : public pipeline::SimpleProcessNode<> {

public:

	SpheresView() {

		registerInput(_spheres, "spheres");
		registerOutput(_painter, "painter");
	}

private:

	void updateOutputs() {

		if (!_painter)
			_painter = new SpheresPainter();

		_painter->setSpheres(_spheres);
	}

	pipeline::Input<Spheres>         _spheres;
	pipeline::Output<SpheresPainter> _painter;
};

#endif // SOPNET_SKELETONS_GUI_SPHERES_VIEW_H__

