#include "playback.hpp"
#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include "persistence.hpp"
#include "rendering.hpp"

namespace GrappleMap
{
	struct VrApp: Vrui::Application
	{
		PlaybackConfig config;
		Graph graph;
		vector<Position> const frames;
		vector<Position>::const_iterator currentFrame = frames.begin();
		Style style;
		ReorientedLocation location{{SegmentInSequence{{0}, 0}, 0}, {}};

		void frame() override;
		void display(GLContextData &) const override;
		void resetNavigation() override;

		VrApp(int argc, char ** argv);
	};

	PlaybackConfig getConfig(int argc, char const * const * const argv)
	{
		optional<PlaybackConfig> const config = playbackConfig_from_args(argc, argv);
		if (!config) abort(); // todo
		return *config;
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, config(getConfig(argc, argv))
		, graph{loadGraph(config.db)}
		, frames{no_titles(prep_frames(config, graph))}
	{
		style.background_color = white;
		style.grid_size = 20;
		style.grid_color = V3{.7, .7, .7};
	}

	void VrApp::frame()
	{
		if (currentFrame != frames.end()) ++currentFrame;

		//double frameTime = Vrui::getCurrentFrameTime(); // todo: use this

		Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
	}

	void VrApp::display(GLContextData &) const
	{
		if (currentFrame == frames.end()) return;

		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_COLOR_MATERIAL);

		setupLights();

		glPushMatrix();
		grid(style.grid_color, style.grid_size, style.grid_line_width);
		render(nullptr, *currentFrame, {}, {}, false);
		glPopMatrix();
	}

	void VrApp::resetNavigation()
	{
		Vrui::NavTransform t=Vrui::NavTransform::identity;
		t *= Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		t *= Vrui::NavTransform::scale(Vrui::getMeterFactor() /* * 0.2*/);
		Vrui::setNavigationTransformation(t);
	}
}

VRUI_APPLICATION_RUN(GrappleMap::VrApp)
