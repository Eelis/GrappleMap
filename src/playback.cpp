#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph.hpp"
#include <GLFW/glfw3.h>
#include <boost/program_options.hpp>
#include <array>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <fstream>

Reorientation sequence_transition(Graph const & graph, SeqNum const from, SeqNum const to)
{
	assert(graph.to(from).node == graph.from(to).node);

	return compose(
		inverse(graph.from(to).reorientation),
		graph.to(from).reorientation);
}

std::vector<std::pair<SeqNum, Reorientation>>
	connectSequences(Graph const & graph, std::vector<SeqNum> const & seqNrs)
{
	std::vector<std::pair<SeqNum, Reorientation>> v;
	Reorientation r = noReorientation();

	for (auto i = seqNrs.begin(); i != seqNrs.end(); )
	{
		v.emplace_back(*i, r);

		auto j = i + 1;

		if (j == seqNrs.end()) break;

		r = compose(sequence_transition(graph, *i, *j), r);
		i = j;
	}

	return v;
}

int main(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	try
	{
		po::options_description desc("options");
		desc.add_options()
			("help", "show this help")
			("frames-per-pos", po::value<unsigned>()->default_value(20), "number of frames rendered per position")
			("script", po::value<std::string>()->default_value("script.txt"), "script file")
			("db", po::value<std::string>()->default_value("positions.txt"), "position database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { std::cout << desc << '\n'; return 0; }

		Graph graph(load(vm["db"].as<std::string>()));
		Camera camera;

		std::vector<SeqNum> seqs;
		{
			std::string filename = vm["script"].as<std::string>();
			std::ifstream f(filename);
			if (!f) throw std::runtime_error(filename + ": " + std::strerror(errno));
			std::string seq;
			while (std::getline(f, seq))
				seqs.push_back(*seq_by_desc(graph, seq));
		}

		if (!glfwInit()) return -1;

		GLFWwindow * const window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);

		if (!window) { glfwTerminate(); return -1; }

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		unsigned const frames_per_position = vm["frames-per-pos"].as<unsigned>();

		foreach(p : connectSequences(graph, seqs))
			for(
				PositionInSequence location = first_pos_in(p.first);
				next(graph, location);
				location = *next(graph, location))
					// See GCC bug 68003 for the reason behind the DRY violation.

				for (unsigned howfar = 0; howfar != frames_per_position; ++howfar)
				{
					camera.rotateHorizontal(-0.01);

					glfwPollEvents();
					if (glfwWindowShouldClose(window)) return 0;

					if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
					if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
					if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { camera.rotateHorizontal(-0.03); }
					if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { camera.rotateHorizontal(0.03); }
					if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
					if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

					Position posToDraw = between(
						apply(p.second, graph[location]),
						apply(p.second, graph[*next(graph, location)]),
						howfar / double(frames_per_position)
						);

					camera.setOffset(xz(posToDraw[0][Core] + posToDraw[1][Core]) / 2);

					renderWindow(
						nullptr, // no viables
						graph, window, posToDraw, camera,
						boost::none, // no highlighted joint
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
