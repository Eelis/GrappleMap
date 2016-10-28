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

using namespace GrappleMap;
namespace po = boost::program_options;

struct Config
{
	string db;
	string script;
	unsigned frames_per_pos;
	unsigned num_transitions;
	string start;
	optional<string /* desc */> demo;
	optional<pair<unsigned, unsigned>> dimensions;
	optional<string> dump;
	optional<uint32_t> seed;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	po::options_description desc("options");
	desc.add_options()
		("help,h", "show this help")
		("frames-per-pos", po::value<unsigned>()->default_value(11),
			"number of frames rendered per position")
		("script", po::value<string>()->default_value(string()),
			"script file")
		("start", po::value<string>()->default_value("staredown"), "initial position")
		("length", po::value<unsigned>()->default_value(50), "number of transitions")
		("dimensions", po::value<string>(), "window dimensions")
		("dump", po::value<string>(), "file to write sequence data to")
		("seed", po::value<uint32_t>(), "PRNG seed")
		("db", po::value<string>()->default_value("GrappleMap.txt"), "database file")
		("demo", po::value<string>(), "show all chains of three transitions that have the given transition in the middle");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { cout << desc << '\n'; return none; }

	optional<pair<unsigned, unsigned>> dimensions;
	if (vm.count("dimensions"))
	{
		string const dims = vm["dimensions"].as<string>();
		auto x = dims.find('x');
		if (x == dims.npos) throw runtime_error("invalid dimensions");
		dimensions = pair<unsigned, unsigned>(std::stoul(dims.substr(0, x)), std::stoul(dims.substr(x + 1)));
	}

	return Config
		{ vm["db"].as<string>()
		, vm["script"].as<string>()
		, vm["frames-per-pos"].as<unsigned>()
		, vm["length"].as<unsigned>()
		, vm["start"].as<string>()
		, optionalopt<string>(vm, "demo")
		, dimensions
		, optionalopt<string>(vm, "dump")
		, optionalopt<uint32_t>(vm, "seed")
		};
}

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


Frames prep_frames(
	Config const & config,
	Graph const & graph)
{
	if (config.demo)
	{
		if (auto step = step_by_desc(graph, *config.demo))
			return demoFrames(graph, *step, config.frames_per_pos);
		else
			throw runtime_error("no such transition: " + *config.demo);
	}
	else if (!config.script.empty())
		return smoothen(frames(graph, readScene(graph, config.script), config.frames_per_pos));
	else if (optional<NodeNum> start = node_by_desc(graph, config.start))
	{
		Frames x = frames(graph, randomScene(graph, *start, config.num_transitions), config.frames_per_pos);

		auto & v = x.front().second;
		auto & w = x.back().second;
		auto const firstpos = v.front();
		auto const lastpos = w.back();
		v.insert(v.begin(), config.frames_per_pos * 5, firstpos);
		w.insert(w.end(), config.frames_per_pos * 15, lastpos);

		return smoothen(x);
	}
	else
		throw runtime_error("no such position/transition: " + config.start);
}


void do_playback(
	Config const & config,
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
					false, // not edit mode
					0, bottom,
					width, height, {0} /* todo */, style);

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

int main(int const argc, char const * const * const argv)
{
	try
	{
		std::srand(std::time(nullptr));

		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

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
				dump(f, p[0]);
				dump(f, p[1]);
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
	catch (exception const & e)
	{
		cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
