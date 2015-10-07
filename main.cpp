#ifndef NDEBUG
#define _GLIBCXX_DEBUG
#endif

#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <map>
#include <algorithm>
#include <fstream>
#include <iterator>

#include <boost/optional.hpp>

using boost::optional;

inline void glVertex(V3 const & v) { glVertex3d(v.x, v.y, v.z); }
inline void glNormal(V3 const & v) { glNormal3d(v.x, v.y, v.z); }
inline void glTranslate(V3 const & v) { glTranslated(v.x, v.y, v.z); }
inline void glColor(V3 v) { glColor3d(v.x, v.y, v.z); }

void triangle(V3 a, V3 b, V3 c)
{
	glVertex(a);
	glVertex(b);
	glVertex(c);
}

void pillar(V3 from, V3 to, double from_radius, double to_radius, unsigned faces)
{
	V3 a = normalize(cross(to - from, V3{1,1,1} - from));
	V3 b = normalize(cross(to - from, a));

	double s = 2 * M_PI / faces;

	glBegin(GL_TRIANGLES);

	for (unsigned i = 0; i != faces; ++i)
	{
		glNormal(a * sin(i * s) + b * cos(i * s));
		glVertex(from + a * from_radius * std::sin( i    * s) + b * from_radius * std::cos( i    * s));
		glVertex(to   + a *   to_radius * std::sin( i    * s) + b *   to_radius * std::cos( i    * s));

		glNormal(a * sin((i + 1) * s) + b * cos((i + 1) * s));
		glVertex(from + a * from_radius * std::sin((i+1) * s) + b * from_radius * std::cos((i+1) * s));

		glVertex(from + a * from_radius * std::sin((i+1) * s) + b * from_radius * std::cos((i+1) * s));
		glVertex(to   + a *   to_radius * std::sin((i+1) * s) + b *   to_radius * std::cos((i+1) * s));

		glNormal(a * sin(i * s) + b * cos(i * s));
		glVertex(to   + a *   to_radius * std::sin( i    * s) + b *   to_radius * std::cos( i    * s));
	}

	glEnd();
}

void render(Player const & p, Segment const s)
{
	auto const a = s.ends[0], b = s.ends[1];
	pillar(p[a], p[b], jointDefs[a].radius, jointDefs[b].radius, 30);
}

void renderWires(Player const & player)
{
	glLineWidth(50);

	foreach (s : segments) if (s.visible) render(player, s);
}

void render(Position const & pos, V3 acolor, V3 bcolor)
{
	Player const & a = pos[0], & b = pos[1];

	glColor(acolor);
	renderWires(a);

	glColor(bcolor);
	renderWires(b);
}

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

	return std::max(0., std::min(1., inner_prod(a, b) / norm2(b) / norm2(b)));
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

	auto const & current_edge = graph.edges[from.sequence];

	foreach (p : viables[j].viables)
	{
		SeqNum const other_seqNum = p.first;
		auto const & viable = p.second;

		if (edit_mode && other_seqNum != from.sequence) continue;

		auto const & other_edge = graph.edges[other_seqNum];

		for (PositionInSequence other{other_seqNum, viable.begin}; other.position != viable.end; ++other.position)
		{
			if (other.sequence == from.sequence)
			{
				if (std::abs(int(other.position) - int(from.position)) != 1)
					continue;
			}
			else
			{
				NodeNum current_node;

				if (from.position == 0) current_node = current_edge.from.node;
				else if (from.position == end(current_edge.sequence) - 1) current_node = current_edge.to.node;
				else continue;

				if (!(current_node == other_edge.to.node
						&& other.position == other_edge.sequence.positions.size() - 2) &&
					!(current_node == other_edge.from.node
						&& other.position == 1))
					continue;
			}

			consider(other, viable.reorientation);
		}
	}

	return np;
}

// state

std::vector<Sequence> sequences = load("positions.txt");
Graph graph = compute_graph(sequences);
PositionInSequence location;
PlayerJoint closest_joint = {0, LeftAnkle};
optional<PlayerJoint> chosen_joint;
GLFWwindow * window;
bool edit_mode = false;
Position clipboard;
Camera camera;
V2 cursor;
double jiggle = 0;
PerPlayerJoint<ViablesForJoint> viable;
Reorientation reorientation;

Sequence & sequence() { return sequences[location.sequence]; }
Position & position() { return graph[location]; }

void grid()
{
	glColor3f(0.5,0.5,0.5);
	glLineWidth(2);

	glBegin(GL_LINES);
		for (double i = -4; i <= 4; ++i)
		{
			glVertex(V3{i/2, 0,  -2});
			glVertex(V3{i/2, 0,   2});
			glVertex(V3{ -2, 0, i/2});
			glVertex(V3{  2, 0, i/2});
		}
	glEnd();
}

void add_position()
{
	if (location.position == sequence().positions.size() - 1)
	{
		sequence().positions.push_back(position());
	}
	else
	{
		auto p = between(position(), sequence().positions[location.position + 1]);
		for(int i = 0; i != 50; ++i)
			spring(p);
		sequence().positions.insert(sequence().positions.begin() + location.position + 1, p);
	}
	++location.position;
}

Position * prev_position()
{
	if (location.position != 0) return &sequence().positions[location.position - 1];
	if (location.sequence != 0) return &sequences[location.sequence - 1].positions.back();
	return nullptr;
}

Position * next_position()
{
	if (location.position != sequence().positions.size() - 1) return &sequence().positions[location.position + 1];
	if (location.sequence != sequences.size() - 1) return &sequences[location.sequence + 1].positions.front();
	return nullptr;
}

void gotoSequence(SeqNum const seq)
{
	if (location.sequence != seq)
	{
		location.sequence = seq;

		std::cout << "Seq: " << sequence().description << std::endl;
	}
}

void key_callback(GLFWwindow * /*window*/, int key, int /*scancode*/, int action, int mods)
{
	if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_C) // copy
	{
		clipboard = position();
		return;
	}

	if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_V) // paste
	{
		position() = clipboard;
		graph = compute_graph(sequences);
		return;
	}

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_INSERT:
				add_position();
				graph = compute_graph(sequences);
				break;

			case GLFW_KEY_PAGE_UP:
				if (location.sequence != 0)
				{
					gotoSequence(location.sequence - 1);
					location.position = 0;
				}
				break;

			case GLFW_KEY_PAGE_DOWN:
				if (location.sequence != sequences.size() - 1)
				{
					gotoSequence(location.sequence + 1);
					location.position = 0;
				}
				break;

			// set position to center

			case GLFW_KEY_U:
				if (auto next = next_position())
				if (auto prev = prev_position())
				{
					position() = between(*prev, *next);
					for(int i = 0; i != 6; ++i) spring(position());
				}
				graph = compute_graph(sequences);
				break;

			// set joint to prev/next/center

			case GLFW_KEY_H:
				if (auto p = prev_position()) position()[closest_joint] = (*p)[closest_joint];
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_K:
				if (auto p = next_position()) position()[closest_joint] = (*p)[closest_joint];
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_J:
				if (auto next = next_position())
				if (auto prev = prev_position())
					position()[closest_joint] = ((*prev)[closest_joint] + (*next)[closest_joint]) / 2;
					for(int i = 0; i != 6; ++i) spring(position());
				break;

			case GLFW_KEY_KP_4:
				for (auto j : playerJoints) position()[j].x -= 0.02;
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_KP_6:
				for (auto j : playerJoints) position()[j].x += 0.02;
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_KP_8:
				for (auto j : playerJoints) position()[j].z -= 0.02;
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_KP_2:
				for (auto j : playerJoints) position()[j].z += 0.02;
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_KP_9:
				for (auto j : playerJoints) position()[j] = xyz(yrot(-0.05) * V4(position()[j], 1));
				graph = compute_graph(sequences);
				break;
			case GLFW_KEY_KP_7:
				for (auto j : playerJoints) position()[j] = xyz(yrot(0.05) * V4(position()[j], 1));
				graph = compute_graph(sequences);
				break;

			// new sequence

			case GLFW_KEY_N:
			{
				auto p = position();
				sequences.push_back(Sequence{"new", {p, p}});
				location.sequence = sequences.size() - 1;
				location.position = 0;
				graph = compute_graph(sequences);
				break;
			}

			case GLFW_KEY_V: edit_mode = !edit_mode; break;

			case GLFW_KEY_S: save(sequences, "positions.txt"); break;
			case GLFW_KEY_DELETE:
			{
				if (mods & GLFW_MOD_CONTROL)
				{
					if (sequences.size() > 1)
					{
						sequences.erase(sequences.begin() + location.sequence);
						if (location.sequence == sequences.size()) --location.sequence;
						location.position = 0;
					}
				}
				else
				{
					if (sequence().positions.size() > 2)
					{
						sequence().positions.erase(sequence().positions.begin() + location.position);
						if (location.position == sequence().positions.size()) --location.position;
					}
				}

				graph = compute_graph(sequences);
				break;
			}
		}
	}
}

void drawJoint(Position const & pos, PlayerJoint pj)
{
	auto color = playerDefs[pj.player].color;
	bool highlight = (pj == (chosen_joint ? *chosen_joint : closest_joint));
	double extraBig = highlight ? 0.01 : 0.005;

	if (edit_mode)
		color = highlight ? yellow : white;
	else if (viable[pj].total_dist == 0)
		extraBig = 0;
	else
		color = highlight
			? white * 0.6 + color * 0.4
			: white * 0.4 + color * 0.6;

	glColor(color);

	static GLUquadricObj * Sphere = gluNewQuadric(); // todo: evil
	glPushMatrix();
		glTranslate(pos[pj]);
		gluSphere(Sphere, jointDefs[pj.joint].radius + extraBig, 20, 20);
	glPopMatrix();
}

void drawJoints(Position const & pos)
{
	foreach (j : playerJoints) drawJoint(pos, j);
}

void determineViables()
{
	foreach (j : playerJoints)
		viable[j] = determineViables(graph, location, j, edit_mode, camera, reorientation);
}

GLfloat light_diffuse[] = {0.5, 0.5, 0.5, 1.0};
GLfloat light_ambient[] = {0.3, 0.3, 0.3, 0.0};

void prepareDraw(int width, int height)
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

void drawViables(PlayerJoint const j)
{
	foreach (v : viable[j].viables)
	{
		if (v.second.end - v.second.begin < 1) continue;

		auto const r = v.second.reorientation;
		auto & seq = sequences[v.first].positions;

		glColor(yellow * 0.9);
		glDisable(GL_DEPTH_TEST);

		glBegin(GL_LINE_STRIP);
		for (unsigned i = v.second.begin; i != v.second.end; ++i) glVertex(apply(r, seq[i][j]));
		glEnd();

		glPointSize(20);
		glBegin(GL_POINTS);
		for (unsigned i = v.second.begin; i != v.second.end; ++i) glVertex(apply(r, seq[i][j]));
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
}

int main()
{
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwSetMouseButtonCallback(window, [](GLFWwindow *, int /*button*/, int action, int /*mods*/)
		{
			if (action == GLFW_PRESS) chosen_joint = closest_joint;
			if (action == GLFW_RELEASE) chosen_joint = boost::none;
		});

	glfwSetScrollCallback(window, [](GLFWwindow * /*window*/, double /*xoffset*/, double yoffset)
		{
			if (yoffset == -1)
			{
				if (location.position != 0) --location.position;
			}
			else if (yoffset == 1)
			{
				if (location.position != sequence().positions.size() - 1) ++location.position;
			}
		});
	
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	optional<NextPosition> next_pos;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.rotateVertical(-0.05);
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.rotateVertical(0.05);
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { camera.rotateHorizontal(-0.03); jiggle = M_PI; }
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { camera.rotateHorizontal(0.03); jiggle = 0; }
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.zoom(-0.05);
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.zoom(0.05);

		if (!edit_mode && !chosen_joint)
		{
			jiggle += 0.01;
			camera.rotateHorizontal(sin(jiggle) * 0.0005);
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		camera.setViewportSize(width, height);

		determineViables();

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		cursor = {((xpos / width) - 0.5) * 2, ((1-(ypos / height)) - 0.5) * 2};

		if (auto best_next_pos = determineNextPos(
				viable, graph, chosen_joint ? *chosen_joint : closest_joint,
				{location.sequence, location.position}, reorientation, camera, cursor, edit_mode))
		{
			double const speed = 0.08;

			if (next_pos)
			{
				if (next_pos->pis == best_next_pos->pis &&
				    next_pos->reorientation == best_next_pos->reorientation)
					next_pos->howfar += std::max(-speed, std::min(speed, best_next_pos->howfar - next_pos->howfar));
				else if (next_pos->howfar > 0.05)
					next_pos->howfar = std::max(0., next_pos->howfar - speed);
				else
					next_pos = NextPosition{best_next_pos->pis, 0, best_next_pos->reorientation};
			}
			else
				next_pos = NextPosition{best_next_pos->pis, 0, best_next_pos->reorientation};
		}

		Position const reorientedPosition = apply(reorientation, position());

		Position posToDraw = reorientedPosition;

		// editing

		if (chosen_joint && edit_mode && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			Position new_pos = position();

			V4 const dragger = yrot(-camera.getHorizontalRotation()) * V4{{1,0,0},0};
			
			auto & joint = new_pos[*chosen_joint];

			auto off = world2xy(camera, joint) - cursor;

			joint.x -= dragger.x * off.x;
			joint.z -= dragger.z * off.x;
			joint.y = std::max(jointDefs[chosen_joint->joint].radius, joint.y - off.y);

			spring(new_pos, chosen_joint);

			posToDraw = apply(reorientation, position() = new_pos);
		}
		else
		{
			if (!chosen_joint)
				closest_joint = *minimal(
					playerJoints.begin(), playerJoints.end(),
					[&](PlayerJoint j) { return norm2(world2xy(camera, reorientedPosition[j]) - cursor); });

			if (next_pos && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
				posToDraw = between(
						reorientedPosition,
						apply(next_pos->reorientation, graph[next_pos->pis]),
						next_pos->howfar);
		}

		auto const center = xz(posToDraw[0][Core] + posToDraw[1][Core]) / 2;

		camera.setOffset(center);

		prepareDraw(width, height);

		glEnable(GL_DEPTH);
		glEnable(GL_DEPTH_TEST);

		glNormal3d(0, 1, 0);
		grid();

		render(posToDraw, red, blue);
		drawJoints(posToDraw);

		glLineWidth(4);
		glNormal3d(0, 1, 0);
		drawViables(chosen_joint ? *chosen_joint : closest_joint);

		if (chosen_joint)
		{
			glDisable(GL_DEPTH_TEST);
			glPointSize(20);
			glBegin(GL_POINTS);
			glColor(white);
			glVertex(posToDraw[*chosen_joint]);
			glEnd();
			glEnable(GL_DEPTH_TEST);
		}

		glfwSwapBuffers(window);
		if (chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && next_pos && next_pos->howfar > 0.95)
		{
			gotoSequence(next_pos->pis.sequence);
			location.position = next_pos->pis.position;
			reorientation = next_pos->reorientation;
			next_pos = boost::none;
		}
	}

	glfwTerminate();
}

/* Edit guidelines:

Divide & conquer: first define the few key positions along the sequence where you have a clear idea of what all the details ought to look like,
then see where the interpolation gives a silly result, e.g. limbs moving through eachother, and add one or more positions in between.

Don't make many small segments, because it makes making significant adjustments later much harder. Let the interpolation do its work.
Make sure there's always at least one big-ish movement going on, because remember: in view mode, small movements are not shown as draggable.

Stay clear from orthogonality. Keep the jiu jitsu tight, so that you don't have limbs flailing around independently.

Todo:
	- multiple viewports
	- in edit mode, make it possible to temporarily hide a player, to unobstruct the view to the other
	- experiment with different joint editing schemes, e.g. direct joint coordinate component manipulation
	- documentation
	- joint rotation limits
	- prevent joints moving through eachother (maybe, might be more annoying than useful)
	- sticky floor
	- direction signs ("to truck", "zombie")
	- undo
	- enhance viability conditions to cut paths short when their xy projection loops or near-loops
	- view from player
	- export graph to DOT
	- recognize reorientations with swapped players
*/
