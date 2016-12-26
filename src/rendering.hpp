#ifndef GRAPPLEMAP_RENDERING_HPP
#define GRAPPLEMAP_RENDERING_HPP

#include "paths.hpp"
#include "viables.hpp"
#include "playerdrawer.hpp"

#ifdef USE_FTGL
#include <FTGL/ftgl.h>
#endif

struct GLFWwindow;

namespace GrappleMap
{
	struct Camera;
	struct Graph;

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
		unsigned grid_size = 2;
		unsigned grid_line_width = 2;

		#ifdef USE_FTGL
			FTGLPixmapFont font{"DejaVuSans.ttf"};
		#endif

		Style();
	};

	#ifdef USE_FTGL
	void renderText(FTGLPixmapFont const &, V2 where, string const &, V3 color);
	#endif

	void setupLights();
	void grid(V3 color, unsigned size = 2, unsigned line_width = 2);

	void renderWindow(vector<View> const &,
		vector<Viable> const &, Graph const &, Position const &,
		Camera, optional<PlayerJoint> highlight_joint,
		PerPlayerJoint<optional<V3>> colors,
		int left, int bottom, int width, int height,
		OrientedPath const &,
		Style const &, PlayerDrawer const &,
		function<void()> extraRender = {});

	void renderScene(Graph const &, Position const &,
		vector<Viable> const & viables,
		optional<PlayerJoint> const browse_joint,
		vector<PlayerJoint> const & edit_joints,
		OrientedPath const &,
		PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> const & accessibleSegments,
		optional<SegmentInSequence> current_segment,
		Style const &,
		PlayerDrawer const &);

	inline vector<View> third_person_windows_in_corner(double w, double h, double hborder, double vborder)
	{
		return
			{ {0, 0, 1, 1, none, 50}
			, {1-w-hborder, vborder, w, h, player0, 80}
			, {hborder, vborder, w, h, player1, 80}
			};
	}
}

#endif
