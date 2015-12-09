#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

#include "math.hpp"
#include "util.hpp"
#include "viables.hpp"

struct Camera;
struct Graph;
struct GLFWwindow;

struct View
{
	double x, y, w, h; // all in [0,1]
	optional<PlayerNum> first_person;
	double fov;
};

struct Style
{
	V3 grid_color {.5, .5, .5};
	V3 background_color {0, 0, 0};
};

void renderWindow(vector<View> const &,
	Viables const *, Graph const &, GLFWwindow *, Position const &,
	Camera, optional<PlayerJoint> highlight_joint, bool edit_mode,
	int width, int height, SeqNum current_sequence,
	Style const & = Style());

#endif
