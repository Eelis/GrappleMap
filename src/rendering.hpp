#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

#include "math.hpp"
#include "viables.hpp"

struct Camera;
struct Graph;
struct GLFWwindow;

struct View
{
	double x, y, w, h; // all in [0,1]
	boost::optional<PlayerNum> first_person;
	double fov;
};

void renderWindow(std::vector<View> const &,
	Viables const *, Graph const &, GLFWwindow *, Position const &,
	Camera, boost::optional<PlayerJoint> highlight_joint, bool edit_mode);

#endif
