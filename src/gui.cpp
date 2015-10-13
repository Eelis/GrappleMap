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
#include <cmath>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stack>

#include <boost/optional.hpp>

using boost::optional;
using std::string;

struct NextPosition
{
	PositionInSequence pis;
	double howfar;
	Reorientation reorientation;
};

double whereBetween(Position const & n, Position const & m, PlayerJoint const j, Camera const & camera, V2 const cursor)
{
	V2 v = world2xy(camera, n[j]);
	V2 w = world2xy(camera, m[j]);

	V2 a = cursor - v;
	V2 b = w - v;

	return std::max(0., /*std::min(1.,*/ inner_prod(a, b) / norm2(b) / norm2(b)/*)*/);
}

optional<NextPosition> determineNextPos(
	PerPlayerJoint<ViablesForJoint> const & viables,
	Graph const & graph, PlayerJoint const j,
	PositionInSequence const from, Reorientation const reorientation,
	Camera const & camera, V2 const cursor, bool const edit_mode)
{
	optional<NextPosition> np;

	double best = 1000000;

	Position const n = apply(reorientation, graph[from]);
	V2 const v = world2xy(camera, n[j]);

	auto consider = [&](PositionInSequence const to, Reorientation const r)
		{
			Position const m = apply(r, graph[to]);

			double const howfar = whereBetween(n, m, j, camera, cursor);

			V2 const w = world2xy(camera, m[j]);

			V2 const ultimate = v + (w - v) * howfar;

			double const d = distanceSquared(ultimate, cursor);

			if (d < best)
			{
				np = NextPosition{to, howfar, r};
				best = d;
			}
		};

	foreach (p : viables[j].viables)
	{
		auto const & viable = p.second;
		PositionInSequence other{p.first, viable.begin};

		if (edit_mode && other.sequence != from.sequence) continue;

		for (; other.position != viable.end; ++other.position)
		{
			if (other.sequence == from.sequence)
			{
				if (std::abs(int(other.position) - int(from.position)) != 1)
					continue;
			}
			else
			{
				if (auto const current_node = node(graph, from))
				{
					auto const & e_to = graph.to(other.sequence);
					auto const & e_from = graph.from(other.sequence);

					if (!(*current_node == e_to.node
							&& other.position == last_pos(graph, other.sequence) - 1) &&
						!(*current_node == e_from.node
							&& other.position == 1))
						continue;
				}
				else continue;
			}
				// todo: clean the above mess up

			consider(other, viable.reorientation);
		}
	}

	return np;
}

struct Window
{
	explicit Window(string const f): filename(f), graph(load(f)) {}

	string filename;
	Graph graph;
	PositionInSequence location{0, 0};
	PlayerJoint closest_joint = {0, LeftAnkle};
	optional<PlayerJoint> chosen_joint;
	bool edit_mode = false;
	optional<Position> clipboard;
	Camera camera;
	double jiggle = 0;
	Viables viable;
	Reorientation reorientation = noReorientation();
	optional<NextPosition> next_pos;
	std::stack<std::pair<Graph, PositionInSequence>> undo;
};

void print_status(Window const & w)
{
	auto && seq = w.graph.sequence(w.location.sequence);

	std::cout
		<< "\r[" << w.location.position + 1
		<< '/' << seq.positions.size() << "] "
		<< seq.description << string(30, ' ') << std::flush;
}

void push_undo(Window & w)
{
	w.undo.emplace(w.graph, w.location);
}

void translate(Window & w, V3 const v)
{
	push_undo(w);
	w.graph.replace(w.location, w.graph[w.location] + v);
}

void key_callback(GLFWwindow * const glfwWindow, int key, int /*scancode*/, int action, int mods)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));

	if (action == GLFW_PRESS)
	{
		if (mods & GLFW_MOD_CONTROL)
		{
			switch (key)
			{
				case GLFW_KEY_Z:
					if (!w.undo.empty())
					{
						std::tie(w.graph, w.location) = w.undo.top();
						w.undo.pop();
						w.next_pos = boost::none;
					}
					return;

				case GLFW_KEY_C: // copy
					w.clipboard = w.graph[w.location];
					return;

				case GLFW_KEY_V: // paste
					if (w.clipboard) w.graph.replace(w.location, *w.clipboard);
					return;
			}

			return;
		}

		switch (key)
		{
			case GLFW_KEY_INSERT:
				push_undo(w);
				w.graph.clone(w.location);
				break;

			case GLFW_KEY_PAGE_UP:
				if (w.location.sequence != 0)
				{
					--w.location.sequence;
					w.location.position = 0;
					w.reorientation = noReorientation();
					print_status(w);
				}
				break;

			case GLFW_KEY_PAGE_DOWN:
				if (w.location.sequence != w.graph.num_sequences() - 1)
				{
					++w.location.sequence;
					w.location.position = 0;
					w.reorientation = noReorientation();
					print_status(w);
				}
				break;

			// swap players

			case GLFW_KEY_X:
			{
				push_undo(w);
				auto p = w.graph[w.location];
				swap(p[0], p[1]);
				w.graph.replace(w.location, p);
				return;
			}

			// set position to center

			case GLFW_KEY_U:
				push_undo(w);
				if (auto nextLoc = next(w.graph, w.location))
				if (auto prevLoc = prev(w.location))
				{
					auto p = between(w.graph[*prevLoc], w.graph[*nextLoc]);
					for(int i = 0; i != 30; ++i) spring(p);
					w.graph.replace(w.location, p);
				}
				break;

			// set joint to prev/next/center

			case GLFW_KEY_H:
				if (auto p = prev(w.location))
				{
					push_undo(w);
					replace(w.graph, w.location, w.closest_joint, w.graph[*p][w.closest_joint]);
				}
				break;
			case GLFW_KEY_K:
				if (auto p = next(w.graph, w.location))
				{
					push_undo(w);
					replace(w.graph, w.location, w.closest_joint, w.graph[*p][w.closest_joint]);
				}
				break;
			case GLFW_KEY_J:
				if (auto prevLoc = prev(w.location))
				if (auto nextLoc = next(w.graph, w.location))
				{
					push_undo(w);
					Position p = w.graph[w.location];
					p[w.closest_joint] = (w.graph[*prevLoc][w.closest_joint] + w.graph[*nextLoc][w.closest_joint]) / 2;
					for(int i = 0; i != 30; ++i) spring(p);
					w.graph.replace(w.location, p);
				}
				break;

			case GLFW_KEY_KP_4: translate(w, V3{-0.02, 0, 0}); break;
			case GLFW_KEY_KP_6: translate(w, V3{0.02, 0, 0}); break;
			case GLFW_KEY_KP_8: translate(w, V3{0, 0, -0.02}); break;
			case GLFW_KEY_KP_2: translate(w, V3{0, 0, 0.02}); break;

			case GLFW_KEY_KP_9:
			{
				push_undo(w);
				auto p = w.graph[w.location];
				foreach (j : playerJoints) p[j] = xyz(yrot(-0.05) * V4(p[j], 1));
				w.graph.replace(w.location, p);
				break;
			}
			case GLFW_KEY_KP_7:
			{
				push_undo(w);
				auto p = w.graph[w.location];
				foreach (j : playerJoints) p[j] = xyz(yrot(0.05) * V4(p[j], 1));
				w.graph.replace(w.location, p);
				break;
			}

			// new sequence

			case GLFW_KEY_N:
			{
				push_undo(w);
				auto const p = w.graph[w.location];
				w.location.sequence = w.graph.insert(Sequence{"new", {p, p}});
				w.location.position = 0;
				break;
			}
			case GLFW_KEY_V: w.edit_mode = !w.edit_mode; break;

			case GLFW_KEY_S: save(w.graph, w.filename); break;

			case GLFW_KEY_DELETE:
			{
				auto const before = std::make_pair(w.graph, w.location);

				if (mods & GLFW_MOD_CONTROL)
				{
					push_undo(w);

					if (auto const new_seq = w.graph.erase_sequence(w.location.sequence))
						w.location = {*new_seq, 0};
					else w.undo.pop();
				}
				else
				{
					push_undo(w);

					if (auto const new_pos = w.graph.erase(w.location))
						w.location.position = *new_pos;
					else w.undo.pop();
				}

				break;
			}
		}
	}
}

GLfloat light_diffuse[] = {0.5, 0.5, 0.5, 1.0};
GLfloat light_ambient[] = {0.3, 0.3, 0.3, 0.0};

void prepareDraw(Camera const & camera, int width, int height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	GLfloat light_position[] = {1.0, 2.0, 1.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(camera.projection().data());

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(camera.model_view().data());
}

void mouse_button_callback(GLFWwindow * const glfwWindow, int const button, int const action, int /*mods*/)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));

	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			push_undo(w);

		w.chosen_joint = w.closest_joint;
	}
	else if (action == GLFW_RELEASE)
		w.chosen_joint = boost::none;
}

void scroll_callback(GLFWwindow * const glfwWindow, double /*xoffset*/, double yoffset)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));

	if (yoffset == -1)
	{
		if (w.location.position != 0) --w.location.position;
	}
	else if (yoffset == 1)
	{
		if (auto const n = next(w.graph, w.location)) w.location = *n;
	}

	print_status(w);
}

int main(int const argc, char const * const * const argv)
{
	if (argc != 2)
	{
		std::cout << "usage: jjm-gui <filename>\n";
		return 1;
	}

	Window w(argv[1]);

	if (!glfwInit()) return -1;

	GLFWwindow * const window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);

	if (!window) { glfwTerminate(); return -1; }

	glfwSetWindowUserPointer(window, &w);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) w.camera.rotateVertical(-0.05);
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) w.camera.rotateVertical(0.05);
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { w.camera.rotateHorizontal(-0.03); w.jiggle = M_PI; }
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { w.camera.rotateHorizontal(0.03); w.jiggle = 0; }
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) w.camera.zoom(-0.05);
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) w.camera.zoom(0.05);

		if (!w.edit_mode && !w.chosen_joint)
		{
			w.jiggle += 0.01;
			w.camera.rotateHorizontal(sin(w.jiggle) * 0.0005);
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		w.camera.setViewportSize(width, height);

		foreach (j : playerJoints)
			w.viable[j] = determineViables(w.graph, w.location, j, w.edit_mode, w.camera, w.reorientation);

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		V2 const cursor = {((xpos / width) - 0.5) * 2, ((1-(ypos / height)) - 0.5) * 2};

		if (auto best_next_pos = determineNextPos(
				w.viable, w.graph, w.chosen_joint ? *w.chosen_joint : w.closest_joint,
				{w.location.sequence, w.location.position}, w.reorientation, w.camera, cursor, w.edit_mode))
		{
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
			{
				double const speed = 0.08;

				if (w.next_pos)
				{
					if (w.next_pos->pis == best_next_pos->pis &&
						w.next_pos->reorientation == best_next_pos->reorientation)
						w.next_pos->howfar += std::max(-speed, std::min(speed, best_next_pos->howfar - w.next_pos->howfar));
					else if (w.next_pos->howfar > 0.05)
						w.next_pos->howfar = std::max(0., w.next_pos->howfar - speed);
					else
						w.next_pos = NextPosition{best_next_pos->pis, 0, best_next_pos->reorientation};
				}
				else
					w.next_pos = NextPosition{best_next_pos->pis, 0, best_next_pos->reorientation};
			}
		}

		Position const reorientedPosition = apply(w.reorientation, w.graph[w.location]);

		Position posToDraw = reorientedPosition;

		// editing

		if (w.chosen_joint && w.edit_mode && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			Position new_pos = w.graph[w.location];

			V4 const dragger = yrot(-w.camera.getHorizontalRotation()) * V4{{1,0,0},0};
			
			auto & joint = new_pos[*w.chosen_joint];

			auto off = world2xy(w.camera, apply(w.reorientation, joint)) - cursor;

			joint.x -= dragger.x * off.x;
			joint.z -= dragger.z * off.x;
			joint.y = std::max(jointDefs[w.chosen_joint->joint].radius, joint.y - off.y);

			spring(new_pos, w.chosen_joint);

			w.graph.replace(w.location, new_pos);

			posToDraw = apply(w.reorientation, new_pos);
		}
		else
		{
			if (w.next_pos)
				posToDraw = between(
						reorientedPosition,
						apply(w.next_pos->reorientation, w.graph[w.next_pos->pis]),
						w.next_pos->howfar);

			if (!w.chosen_joint)
				w.closest_joint = *minimal(
					playerJoints.begin(), playerJoints.end(),
					[&](PlayerJoint j) { return norm2(world2xy(w.camera, posToDraw[j]) - cursor); });
		}

		auto const center = xz(posToDraw[0][Core] + posToDraw[1][Core]) / 2;

		w.camera.setOffset(center);

		prepareDraw(w.camera, width, height);

		glEnable(GL_DEPTH);
		glEnable(GL_DEPTH_TEST);

		glNormal3d(0, 1, 0);
		grid();

		auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

		render(w.viable, posToDraw, special_joint, w.edit_mode);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLineWidth(4);
		glNormal3d(0, 1, 0);
		drawViables(w.graph, w.viable, special_joint);

		glDisable(GL_DEPTH_TEST);
		glPointSize(20);
		glColor(white);

		glBegin(GL_POINTS);
		glVertex(posToDraw[special_joint]);
		glEnd();

		glfwSwapBuffers(window);

		if (w.chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && w.next_pos && w.next_pos->howfar >= 1)
		{
			w.location = w.next_pos->pis;
			w.reorientation = w.next_pos->reorientation;
			w.next_pos = boost::none;
			print_status(w);
		}
	}

	glfwTerminate();

	std::cout << '\n';
}
