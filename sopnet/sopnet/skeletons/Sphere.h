#ifndef SOPNET_SKELETONS_SPHERE_H__
#define SOPNET_SKELETONS_SPHERE_H__

class Sphere {

public:

	Sphere(float x, float y, float z, float r) :
		_x(x),
		_y(y),
		_z(z),
		_r(r) {}

	void setX(float x) { _x = x; }
	void setY(float y) { _y = y; }
	void setZ(float z) { _z = z; }

	void setRadius(float radius) { _r = radius; }

	float getX() const { return _x; }
	float getY() const { return _y; }
	float getZ() const { return _z; }

	float getRadius() const { return _r; }

private:

	float _x;
	float _y;
	float _z;
	float _r;
};

#endif // SOPNET_SKELETONS_SPHERE_H__

