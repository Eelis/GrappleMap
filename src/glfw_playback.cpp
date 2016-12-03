#include "playback.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph_util.hpp"
#include "paths.hpp"
#include <unistd.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <iostream>
#include <vector>
#include <fstream>

namespace GrappleMap
{
	void dump(std::ostream & o, PerJoint<V3> const & p)
	{
		foreach(j : {
			Core, Neck, Head,
			LeftHip, LeftKnee, LeftAnkle, LeftHeel, LeftToe,
			LeftShoulder, LeftElbow, LeftWrist, LeftHand, LeftFingers,
			RightHip, RightKnee, RightAnkle, RightHeel, RightToe,
			RightShoulder, RightElbow, RightWrist, RightHand, RightFingers})
		{
			o << p[j].x << ' ' << p[j].y << ' ' << p[j].z << ' ';
		}
	}

	void do_playback(
		PlaybackConfig const & config,
		Graph const & graph,
		GLFWwindow * const window)
	{
		for (;;)
		{
			Frames const fr = prep_frames(config, graph);

			Camera camera;
			Style style;
			style.background_color = white;
			style.grid_size = 20;
			style.grid_color = V3{.7, .7, .7};
			camera.zoom(1.2);
			camera.hardSetOffset(cameraOffsetFor(fr.front().second.front()));

			PlayerDrawer playerDrawer;

			string const separator = "      ";

			for (auto i = fr.begin(); i != fr.end(); ++i)
			{
				#ifdef USE_FTGL
					double const textwidth = style.font.Advance((i->first + separator).c_str(), -1);
					V2 textpos{10,20};

					string caption = i->first;
					for (auto j = i+1; j != i + std::min(fr.end() - i, 6l); ++j)
						caption += separator + j->first;
				#endif

				foreach (pos : i->second)
				{
					glfwPollEvents();
					if (glfwWindowShouldClose(window)) return;

					camera.rotateHorizontal(-0.013);
					camera.setOffset(cameraOffsetFor(pos));

					if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
					if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
					if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.rotateHorizontal(-0.03);
					if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotateHorizontal(0.03);
					if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
					if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

					int bottom = 0;
					int width, height;
					glfwGetFramebufferSize(window, &width, &height);

					if (config.dimensions)
					{
						width = config.dimensions->first;

						bottom = height - config.dimensions->second;
						height = config.dimensions->second;
					}

					renderWindow(
						{{0, 0, 1, 1, none, 50}},
		//				third_person_windows_in_corner(.3,.3,.01,.01 * (double(width)/height)),
						nullptr, // no viables
						graph, pos, camera,
						none, // no highlighted joint
						{}, // default colors
						0, bottom,
						width, height, {}, style, playerDrawer);

					/*
					#ifdef USE_FTGL
						renderText(style.sequenceFont, textpos, caption, black);
						textpos.x -= textwidth / (i->second.size()-1);
					#endif
					*/

					glfwSwapBuffers(window);
				}

			}
		}
	}

	void run(int const argc, char const * const * const argv)
	{
		std::srand(std::time(nullptr));

		optional<PlaybackConfig> const config = playbackConfig_from_args(argc, argv);
		if (!config) return;

		std::srand(config->seed ? *config->seed : std::time(nullptr));

		Graph const graph = loadGraph(config->db);

		if (config->dump)
		{
			Frames const frames = prep_frames(*config, graph);
			int n = 0;
			std::ofstream f(*config->dump);

			foreach (x : frames)
			foreach (p : x.second)
			{
				dump(f, p[player0]);
				dump(f, p[player1]);
				++n;
			}

			std::cout << "Wrote " << n << " frames to " << *config->dump << '\n';
		}
		else
		{
			if (!glfwInit()) error("could not initialize GLFW");

			GLFWwindow * const window = glfwCreateWindow(640, 480, "GrappleMap", nullptr, nullptr);
			if (!window) error("could not create window");

			glfwMakeContextCurrent(window);
			glfwSwapInterval(1);

			do_playback(*config, graph, window);

			glfwTerminate();
		}
	}
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		GrappleMap::run(argc, argv);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
