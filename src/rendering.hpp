#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

#include "math.hpp"
#include "viables.hpp"

struct Camera;
struct Graph;
struct GLFWwindow;

void renderWindow(
	Viables const *, Graph const &, GLFWwindow *, Position const &,
	Camera, boost::optional<PlayerJoint> highlight_joint, bool edit_mode);

#endif
