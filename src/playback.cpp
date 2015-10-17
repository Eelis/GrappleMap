#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <iterator>

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
	if (argc != 2)
	{
		std::cout << "usage: jjm-playback <positions-file> < <script-file>\n";
		return 1;
	}

	Graph graph(load(argv[1]));
	Camera camera;

	if (!glfwInit()) return -1;

	GLFWwindow * const window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);

	if (!window) { glfwTerminate(); return -1; }

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	std::vector<SeqNum> seqs;

	{
		std::string seq;
		while (std::getline(std::cin, seq))
			seqs.push_back(*seq_by_desc(graph, seq));
	}

	unsigned const frames_per_position = 12;

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

				if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
				if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
				if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { camera.rotateHorizontal(-0.03); }
				if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { camera.rotateHorizontal(0.03); }
				if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
				if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

				int width, height;
				glfwGetFramebufferSize(window, &width, &height);
				camera.setViewportSize(width, height);

				Position posToDraw = between(
					apply(p.second, graph[location]),
					apply(p.second, graph[*next(graph, location)]),
					howfar / double(frames_per_position)
					);

				auto const center = xz(posToDraw[0][Core] + posToDraw[1][Core]) / 2;

				camera.setOffset(center);

				prepareDraw(camera, width, height);

				glEnable(GL_DEPTH);
				glEnable(GL_DEPTH_TEST);

				glNormal3d(0, 1, 0);
				grid();

				render(
					nullptr, // no viables
					posToDraw,
					boost::none, // no highlighted joint
					false); // not edit mode

				glfwSwapBuffers(window);
			}

	sleep(2);

	glfwTerminate();
}
