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
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace GrappleMap;

struct Config
{
	string db;
	string script;
	unsigned frames_per_pos;
	unsigned num_transitions;
	string start;
	optional<string /* desc */> demo;
	optional<pair<unsigned, unsigned>> dimensions;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help,h", "show this help")
		("frames-per-pos", po::value<unsigned>()->default_value(9),
			"number of frames rendered per position")
		("script", po::value<string>()->default_value(string()),
			"script file")
		("start", po::value<string>()->default_value("deep half"), "initial node (only used if no script given)")
		("length", po::value<unsigned>()->default_value(50), "number of transitions")
		("dimensions", po::value<string>(), "window dimensions")
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
		, vm.count("demo") ? optional<string>(vm["demo"].as<string>()) : boost::none
		, dimensions
		};
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		std::srand(std::time(nullptr));

		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		Graph const graph = loadGraph(config->db);

		Frames fr;

		if (config->demo)
		{
			if (auto step = step_by_desc(graph, *config->demo))
				fr = demoFrames(graph, *step, config->frames_per_pos);
			else
				throw runtime_error("no such transition: " + *config->demo);
		}
		else if (!config->script.empty())
			fr = smoothen(frames(graph, readScene(graph, config->script), config->frames_per_pos));
		else if (optional<NodeNum> start = node_by_desc(graph, config->start))
			fr = smoothen(frames(graph, randomScene(graph, *start, config->num_transitions), config->frames_per_pos));
//		else if (optional<SeqNum> start = seq_by_desc(graph, config->start))
//			fr = frames(graph, Scene{randomScene(graph, *start, config->num_transitions)}, config->frames_per_pos);
		else
			throw runtime_error("no such position/transition: " + config->start);

		if (!glfwInit()) error("could not initialize GLFW");

		GLFWwindow * const window = glfwCreateWindow(640, 480, "GrappleMap", nullptr, nullptr);
		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		Camera camera;
		Style style;
		style.background_color = white;
		style.grid_size = 4;
		style.grid_color = V3{.7, .7, .7};
		camera.zoom(1.2);
		//camera.zoom(0.9);

		string const separator = "      ";

		for (auto i = fr.begin(); i != fr.end(); ++i)
		{
			#ifdef USE_FTGL
				double const textwidth = style.sequenceFont.Advance((i->first + separator).c_str(), -1);
				V2 textpos{10,20};

				string caption = i->first;
				for (auto j = i+1; j != i + std::min(fr.end() - i, 6l); ++j)
					caption += separator + j->first;
			#endif

			foreach (pos : i->second)
			{
				glfwPollEvents();
				if (glfwWindowShouldClose(window)) return 0;

				camera.rotateHorizontal(-0.013);
				camera.setOffset(xz(between(pos[0][Core], pos[1][Core])));

				if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
				if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
				if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.rotateHorizontal(-0.03);
				if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotateHorizontal(0.03);
				if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
				if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

				int bottom = 0;
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				if (config->dimensions)
				{
					width = config->dimensions->first;

					bottom = height - config->dimensions->second;
					height = config->dimensions->second;
				}

				renderWindow(
					{{0, 0, 1, 1, none, 50}},
//					third_person_windows_in_corner(.3,.3,.02),
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

		sleep(2);

		glfwTerminate();
	}
	catch (exception const & e)
	{
		cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
