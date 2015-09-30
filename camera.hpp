#ifndef JIUJITSUMAPPER_CAMERA_HPP
#define JIUJITSUMAPPER_CAMERA_HPP

#include "math.hpp"

class Camera
{
	V2 viewportSize;
	V3 orientation{0, -0.7, 1.7};
		// x used for rotation over y axis, y used for rotation over x axis, z used for zoom
	M proj, mv, full;

	void computeMatrices()
	{
		proj = perspective(90, viewportSize.x / viewportSize.y, 0.1, 6);
		mv = translate(0, 0, -orientation.z) * xrot(orientation.y) * yrot(orientation.x);
		full = proj * mv;
	}

public:

	Camera() { computeMatrices(); }

	M const & projection() const { return proj; }
	M const & model_view() const { return mv; }

	V2 world2xy(V3 v) const
	{
		auto cs = full * V4(v, 1);
		return xy(cs) / cs.w;
	}

	void setViewportSize(int x, int y)
	{
		viewportSize.x = x;
		viewportSize.y = y;
		computeMatrices();
	}

	void rotateHorizontal(double radians)
	{
		orientation.x += radians;
		computeMatrices();
	}

	void rotateVertical(double radians)
	{
		orientation.y += radians;
		orientation.y = std::max(-M_PI/2, orientation.y);
		orientation.y = std::min(M_PI/2, orientation.y);
		computeMatrices();
	}

	void zoom(double z)
	{
		orientation.z += z;
		computeMatrices();
	}

	double getHorizontalRotation() const { return orientation.x; }

};

#endif
