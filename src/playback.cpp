#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph.hpp"
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <fstream>

PositionReorientation sequence_transition(Graph const & graph, SeqNum const from, SeqNum const to)
{
	assert(graph.to(from).node == graph.from(to).node);

	return compose(
		inverse(graph.from(to).reorientation),
		graph.to(from).reorientation);
}

vector<ReorientedSequence> connectSequences(Graph const & graph, vector<SeqNum> const & seqNrs)
{
	vector<ReorientedSequence> v;
	PositionReorientation r;

	for (auto i = seqNrs.begin(); i != seqNrs.end(); )
	{
		v.push_back({*i, r});

		auto j = i + 1;

		if (j == seqNrs.end()) break;

		r = compose(sequence_transition(graph, *i, *j), r);
		i = j;
	}

	return v;
}

vector<Position> frames(Graph const & graph, vector<SeqNum> const & seqs, unsigned const frames_per_pos)
{
	vector<Position> r;

	for (ReorientedSequence const & rs : connectSequences(graph, seqs))
		for (
			PositionInSequence location = first_pos_in(rs.sequence);
			next(graph, location);
			location = *next(graph, location))
				// See GCC bug 68003 for the reason behind the DRY violation.

			for (unsigned howfar = 0; howfar != frames_per_pos; ++howfar)
				r.push_back(between(
					rs.reorientation(graph[location]),
					rs.reorientation(graph[*next(graph, location)]),
					howfar / double(frames_per_pos)));

	return r;
}

struct Config
{
	string db;
	string script;
	unsigned frames_per_pos;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help", "show this help")
		("frames-per-pos", po::value<unsigned>()->default_value(20),
			"number of frames rendered per position")
		("script", po::value<string>()->default_value("script.txt"),
			"script file")
		("db", po::value<string>()->default_value("positions.txt"),
			"position database file");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { std::cout << desc << '\n'; return none; }

	return Config
		{ vm["db"].as<string>()
		, vm["script"].as<string>()
		, vm["frames-per-pos"].as<unsigned>() };
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		Graph const graph(load(config->db));
		vector<SeqNum> const seqs = readScript(graph, config->script);

		if (!glfwInit()) error("could not initialize GLFW");

		GLFWwindow * const window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);
		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		Camera camera;

		for (Position const & pos : frames(graph, seqs, config->frames_per_pos))
		{
			glfwPollEvents();
			if (glfwWindowShouldClose(window)) return 0;

			camera.rotateHorizontal(-0.005);
			camera.setOffset(xz(between(pos[0][Core], pos[1][Core])));

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.rotateHorizontal(-0.03);
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotateHorizontal(0.03);
			if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
			if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

			renderWindow(
				// views:
				{ {0, 0, 1, 1, none, 90}
				, {1-.3-.02, .02, .3, .3, optional<unsigned>(0), 60}
				, {.02, .02, .3, .3, optional<unsigned>(1), 60} },

				nullptr, // no viables
				graph, window, pos, camera,
				none, // no highlighted joint
				false); // not edit mode
		}

		sleep(2);

		glfwTerminate();
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
