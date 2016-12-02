#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph_util.hpp"
#include <GLFW/glfw3.h>
#include <boost/program_options.hpp>
#include <cmath>
#include <GL/glu.h>
#include <numeric>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <stack>

using namespace GrappleMap;

struct NextPosition
{
	PositionInSequence pis;
	double howfar;
	PositionReorientation reorientation;
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
	PositionInSequence const from, PositionReorientation const reorientation,
	Camera const & camera, V2 const cursor, bool const edit_mode)
{
	optional<NextPosition> np;

	double best = 1000000;

	Position const n = reorientation(graph[from]);
	V2 const v = world2xy(camera, n[j]);

	auto consider = [&](PositionInSequence const to, PositionReorientation const r)
		{
			Position const m = r(graph[to]);

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
				if (std::abs(int(other.position.index) - int(from.position.index)) != 1)
					continue;
			}
			else
			{
				if (auto const current_node = node(graph, from))
				{
					auto const & e_to = graph.to(other.sequence);
					auto const & e_from = graph.from(other.sequence);

					if (!(current_node->node == e_to.node
							&& other.position == *prev(last_pos(graph[other.sequence]))) &&
						!(current_node->node == e_from.node
							&& other.position == PosNum{1}))
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
	explicit Window(string const f)
		: filename(f), graph(loadGraph(filename))
	{}

	string const filename;
	Graph graph;
	PositionInSequence location{{0}, {0}};
	PlayerJoint closest_joint = {{0}, LeftAnkle};
	optional<PlayerJoint> chosen_joint;
	bool edit_mode = false;
	optional<Position> clipboard;
	Camera camera;
	bool split_view = false;
	double jiggle = 0;
	Viables viable;
	PositionReorientation reorientation{};
	optional<NextPosition> next_pos;
	double last_cursor_x = 0, last_cursor_y = 0;
	std::stack<std::pair<Graph, PositionInSequence>> undo;
	Style style;
	PlayerDrawer playerDrawer;

	std::map<NodeNum, unsigned> anim_next;
		// the unsigned is an index into out(graph, n)
};

void print_status(Window const & w)
{
	auto && seq = w.graph[w.location.sequence];

	std::cout
		<< "\r[" << next(w.location.position)
		<< '/' << seq.positions.size() << "] "
		<< seq.description.front() << string(30, ' ') << std::flush;
}

void push_undo(Window & w)
{
	w.undo.emplace(w.graph, w.location);
}

void translate(Window & w, V3 const v)
{
	push_undo(w);
	w.graph.replace(w.location, w.graph[w.location] + v, true);
}

void key_callback(GLFWwindow * const glfwWindow, int key, int /*scancode*/, int action, int mods)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));

	double const translation = mods & GLFW_MOD_SHIFT ? 0.2 : 0.02;

	if (action == GLFW_PRESS)
	{
		if (mods & GLFW_MOD_CONTROL)
			switch (key)
			{
				case GLFW_KEY_Z:
					if (!w.undo.empty())
					{
						std::tie(w.graph, w.location) = w.undo.top();
						w.undo.pop();
						w.next_pos = none;
					}
					return;

				case GLFW_KEY_C: // copy
					w.clipboard = w.graph[w.location];
					return;

				case GLFW_KEY_V: // paste
					if (w.clipboard) w.graph.replace(w.location, *w.clipboard, true);
					return;

				case GLFW_KEY_UP:
				{
					NodeNum const n = w.graph.from(w.location.sequence).node;

					vector<SeqNum> const v = out_sequences(w.graph, n);
					if (v.empty()) return;
					unsigned & an = w.anim_next[n];
					an = (an == 0 ? v.size() - 1 : an - 1);
					return;
				}

				case GLFW_KEY_DOWN:
				{
					NodeNum const n = w.graph.from(w.location.sequence).node;

					vector<SeqNum> const v = out_sequences(w.graph, n);
					if (!v.empty())
						++w.anim_next[n] %= v.size();
					return;
				}

				default: return;
			}
		else
			switch (key)
			{
				case GLFW_KEY_INSERT:
					push_undo(w);
					w.graph.clone(w.location);
					break;

				case GLFW_KEY_PAGE_UP:
					if (auto x = prev(w.location.sequence))
					{
						w.location.sequence = *x;
						w.location.position.index = 0;
						w.reorientation = PositionReorientation();
						print_status(w);
					}
					break;

				case GLFW_KEY_PAGE_DOWN:
					if (int32_t(w.location.sequence.index) != w.graph.num_sequences() - 1)
					{
						++w.location.sequence;
						w.location.position.index = 0;
						w.reorientation = PositionReorientation();
						print_status(w);
					}
					break;

				// swap players

				case GLFW_KEY_X:
				{
					push_undo(w);
					auto p = w.graph[w.location];
					swap_players(p);
					w.graph.replace(w.location, p, true);
					break;
				}

				// mirror

				case GLFW_KEY_M:
				{
					push_undo(w);
					w.graph.replace(w.location, mirror(w.graph[w.location]), true);
					break;
				}

				// set position to center

				case GLFW_KEY_U:
					push_undo(w);
					if (auto nextLoc = next(w.graph, w.location))
					if (auto prevLoc = prev(w.location))
					{
						auto p = between(w.graph[*prevLoc], w.graph[*nextLoc]);
						for(int i = 0; i != 30; ++i) spring(p);
						w.graph.replace(w.location, p, true);
					}
					break;


				case GLFW_KEY_I:
					w.reorientation.mirror = !w.reorientation.mirror;
					break;

				// set joint to prev/next/center

				case GLFW_KEY_H:
					if (auto p = prev(w.location))
					{
						push_undo(w);
						auto const j = apply(w.reorientation, w.closest_joint);
						replace(w.graph, w.location, j, w.graph[*p][j], false);
					}
					break;
				case GLFW_KEY_K:
					if (auto p = next(w.graph, w.location))
					{
						push_undo(w);
						auto const j = apply(w.reorientation, w.closest_joint);
						replace(w.graph, w.location, j, w.graph[*p][j], false);
					}
					break;
				case GLFW_KEY_J:
					if (auto prevLoc = prev(w.location))
					if (auto nextLoc = next(w.graph, w.location))
					{
						push_undo(w);
						Position p = w.graph[w.location];
						auto const j = apply(w.reorientation, w.closest_joint);
						p[j] = (w.graph[*prevLoc][j] + w.graph[*nextLoc][j]) / 2;
						for(int i = 0; i != 30; ++i) spring(p);
						w.graph.replace(w.location, p, false);
					}
					break;

				case GLFW_KEY_KP_4: translate(w, V3{-translation, 0, 0}); break;
				case GLFW_KEY_KP_6: translate(w, V3{translation, 0, 0}); break;
				case GLFW_KEY_KP_8: translate(w, V3{0, 0, -translation}); break;
				case GLFW_KEY_KP_2: translate(w, V3{0, 0, translation}); break;

				case GLFW_KEY_KP_9:
				{
					push_undo(w);
					auto p = w.graph[w.location];
					foreach (j : playerJoints) p[j] = yrot(-0.05) * p[j];
					w.graph.replace(w.location, p, true);
					break;
				}
				case GLFW_KEY_KP_7:
				{
					push_undo(w);
					auto p = w.graph[w.location];
					foreach (j : playerJoints) p[j] = yrot(0.05) * p[j];
					w.graph.replace(w.location, p, true);
					break;
				}

				// new sequence

				case GLFW_KEY_N:
				{
					push_undo(w);
					auto const p = w.graph[w.location];
					w.location.sequence = insert(w.graph, Sequence{{"new"}, {p, p}, none});
					w.location.position.index = 0;
					break;
				}
				case GLFW_KEY_V: w.edit_mode = !w.edit_mode; break;

				case GLFW_KEY_S: save(w.graph, w.filename); break;

				case GLFW_KEY_1: w.split_view = !w.split_view; break;

				case GLFW_KEY_B: // branch
				{
					push_undo(w);

					try { split_at(w.graph, w.location); }
					catch (std::exception const & e)
					{
						std::cerr << "split_at: " << e.what() << '\n';
					}

					break;
				}

				case GLFW_KEY_DELETE:
				{
					auto const before = make_pair(w.graph, w.location);

					if (mods & GLFW_MOD_CONTROL)
					{
						push_undo(w);

						if (auto const new_seq = erase_sequence(w.graph, w.location.sequence))
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
		w.chosen_joint = none;
}

void scroll_callback(GLFWwindow * const glfwWindow, double /*xoffset*/, double yoffset)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));

	if (yoffset == -1)
	{
		if (auto pp = prev(w.location.position))
		{
			w.location.position = *pp;
			w.next_pos = none;
		}
	}
	else if (yoffset == 1)
	{
		if (auto const n = next(w.graph, w.location))
		{
			w.location = *n;
			w.next_pos = none;
		}
	}

	print_status(w);
}

std::vector<View> const
	single_view
		{ {0, 0, 1, 1, none, 90} },
	split_view
		{ {0, 0, .5, 1, none, 90}
		, {.5, .5, .5, .5, player0, 75}
		, {.5, 0, .5, .5, player1, 75} };

View const * main_view(std::vector<View> const & vv)
{
	foreach (v : vv) if (!v.first_person) return &v;
	return nullptr;
}

void backward(Window &)
{
	// TODO
}

void forward(Window & w)
{
	if (w.next_pos)
	{
		// todo: what if we're going backwards?

		w.next_pos->howfar += 0.08;

		if (w.next_pos->howfar >= 1)
		{
			w.location = w.next_pos->pis;
			w.reorientation = w.next_pos->reorientation;

			double const hf = w.next_pos->howfar - 1;

			w.next_pos = none;

			if (w.location == last_pos_in(w.graph, w.location.sequence))
			{
				auto const n = w.graph.to(w.location.sequence).node;
				auto const v = out_sequences(w.graph, n);

				if (!v.empty())
				{
					PositionReorientation r;

					SeqNum const next_seq = v[w.anim_next[n]];

					#ifndef NDEBUG
						Position const before = w.reorientation(w.graph[w.location]);
						Position const after = r(w.graph[first_pos_in(next_seq)]);
					#endif

					assert(basicallySame(
						w.reorientation(w.graph[w.location]),
						r(w.graph[first_pos_in(next_seq)])));

					w.next_pos = NextPosition{{next_seq, 1}, hf, r};
				}
			}
			else w.next_pos = NextPosition{{w.location.sequence, next(w.location.position)}, hf, {}};

			print_status(w);
		}
	}
}

void cursor_pos_callback(GLFWwindow * const window, double const xpos, double const ypos)
{
	Window & w = *reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

	if (w.last_cursor_x != 0 && w.last_cursor_y != 0 &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS)
	{
		double const speed = 0.002;

		w.camera.rotateVertical((ypos - w.last_cursor_y) * -speed);
		w.camera.rotateHorizontal((xpos - w.last_cursor_x) * speed);
	}

	w.last_cursor_x = xpos;
	w.last_cursor_y = ypos;
}

template <typename T>
T maybe_mirror(bool b, T x)
{
	return b ? -x : x;
}

string const usage =
	"Usage: grapplemap-editor [OPTIONS] [START]\n";

string const start_desc =
	"START can be:\n"
	"  - \"p#\" for position #\n"
	"  - \"t#\" for transition #\n"
	"  - the first line of a position or transition's description,\n"
	"    with newlines replaced with spaces\n"
	"  - \"last-trans\" for the last transition in the database\n";

string const controls =
	"Keys:\n"
	"  arrow keys - rotate camera\n"
	"  home/end   - zoom in/out\n"
	"  v          - switch between edit and explore mode\n"
	"  s          - write database to file\n"
	"  h          - set position of highlighted joint to\n"
	"                what it was in the previous position\n"
	"  j          - set position of highlighted joint to\n"
	"                interpolated midpoint\n"
	"  k          - set position of highlighted joint to\n"
	"                what it was in the next position\n"
	"  u          - make current position the interpolated midpoint\n"
	"                between the previous and next position\n"
	"  x          - swap players\n"
	"  m          - mirror position\n"
	"  i          - mirror view\n"
	"  n          - create new transition with start and end point identical to current frame\n"
	"  b          - split current transition into two at current frame, where the latter becomes a new node\n"
	"  numpad 7/9  - rotate position\n"
	"  numpad arrows - move position\n"
	"  ctrl-c     - copy position to clipboard\n"
	"  ctrl-v     - paste position from clipboard, overwriting current position\n"
	"  ctrl-z     - undo\n"
	"  page up    - go to previous transition\n"
	"  page down  - go to next transition\n"
	"  ctrl-del   - delete current transition\n"
	"  ins        - duplicate current frame\n"
	"  del        - delete current frame\n"
	"\n"
	"Mouse controls:\n"
	"  scroll wheel              - scroll through frames in sequence\n"
	"  right click + drag joint  - move joint along path\n"
	"  middle click + move       - rotate camera\n"
	"  left click + drag joint   - move joint (only in edit mode)\n";

int main(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	try
	{
		po::options_description optdesc("Options");
		optdesc.add_options()
			("help,h", "show this help")
			("start", po::value<string>()->default_value("last-trans"), "see START below")
			("db", po::value<string>()->default_value("GrappleMap.txt"), "database file");

		po::positional_options_description posopts;
		posopts.add("start", -1);

		po::variables_map vm;
		po::store(
			po::command_line_parser(argc, argv)
				.options(optdesc)
				.positional(posopts)
				.run(),
			vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			std::cout << usage << '\n' << optdesc << '\n' << start_desc << '\n' << controls;
			return 0;
		}

		Window w(vm["db"].as<std::string>());

		if (auto start = posinseq_by_desc(w.graph, vm["start"].as<string>()))
			w.location = *start;
		else
			throw runtime_error("no such position/transition");

		if (!glfwInit()) return -1;

		GLFWwindow * const window = glfwCreateWindow(640, 480, "GrappleMap", nullptr, nullptr);

		if (!window) { glfwTerminate(); return -1; }

		glfwSetWindowUserPointer(window, &w);

		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetCursorPosCallback(window, cursor_pos_callback);

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) w.camera.rotateVertical(-0.05);
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) w.camera.rotateVertical(0.05);
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { w.camera.rotateHorizontal(0.03); w.jiggle = pi(); }
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { w.camera.rotateHorizontal(-0.03); w.jiggle = 0; }
			if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) w.camera.zoom(-0.05);
			if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) w.camera.zoom(0.05);
			if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) forward(w);
			if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) backward(w);

			if (!w.edit_mode && !w.chosen_joint)
			{
				w.jiggle += 0.01;
				w.camera.rotateHorizontal(sin(w.jiggle) * 0.0005);
			}

			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			optional<V2> cursor;

			auto & views = w.split_view ? split_view : single_view;

			if (View const * v = main_view(views))
			{
				w.camera.setViewportSize(v->fov, v->w * width, v->h * height);

				foreach (j : playerJoints)
					w.viable[j] = determineViables(w.graph, w.location, j, &w.camera, w.reorientation);

				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);

				double x = xpos / width;
				double y = 1 - (ypos / height);

				if (x >= v->x && x <= v->x + v->w &&
					y >= v->y && y <= v->y + v->h)
					cursor = V2{
						(((x - v->x) / v->w) - 0.5) * 2,
						(((y - v->y) / v->h) - 0.5) * 2};
			}

			if (cursor)
				if (auto best_next_pos = determineNextPos(
						w.viable, w.graph, w.chosen_joint ? *w.chosen_joint : w.closest_joint,
						{w.location.sequence, w.location.position}, w.reorientation, w.camera, *cursor, w.edit_mode))
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

			Position const reorientedPosition = w.reorientation(w.graph[w.location]);

			Position posToDraw = reorientedPosition;

			// editing

			if (cursor && w.chosen_joint && w.edit_mode && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				Position new_pos = w.graph[w.location];

				V3 dragger = [&]{
						PositionReorientation r;
						r.reorientation.angle = -w.camera.getHorizontalRotation();
						return compose(w.reorientation, r)(V3{1,0,0});
					}();
				V3 const v = apply(w.reorientation, new_pos, *w.chosen_joint);
				V2 const joint_xy = world2xy(w.camera, v);

				double const
					offx = (cursor->x - joint_xy.x)
						/ (world2xy(w.camera, v + dragger).x - joint_xy.x),
					offy = (cursor->y - joint_xy.y)
						* 0.01 / (world2xy(w.camera, v + V3{0,0.01,0}).y - joint_xy.y);

				auto const rj = apply(w.reorientation, *w.chosen_joint);

				auto & joint = new_pos[rj];

				if (w.reorientation.mirror) dragger.x = -dragger.x;

				joint.x = std::max(-2., std::min(2., joint.x + dragger.x * offx));
				joint.z = std::max(-2., std::min(2., joint.z + dragger.z * offx));
				joint.y = std::max(jointDefs[w.chosen_joint->joint].radius, joint.y + offy);

				spring(new_pos, rj);

				w.graph.replace(w.location, new_pos, false);

				posToDraw = w.reorientation(new_pos);
			}
			else
			{
				if (w.next_pos && (!w.edit_mode || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS))
					posToDraw = between(
							reorientedPosition,
							w.next_pos->reorientation(w.graph[w.next_pos->pis]),
							w.next_pos->howfar);

				if (cursor && !w.chosen_joint)
					w.closest_joint = *minimal(
						playerJoints.begin(), playerJoints.end(),
						[&](PlayerJoint j) { return norm2(world2xy(w.camera, posToDraw[j]) - *cursor); });
			}

			auto const center = xz(posToDraw[player0][Core] + posToDraw[player1][Core]) / 2;

			w.camera.setOffset(center);

			auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

			glfwGetFramebufferSize(window, &width, &height);
			renderWindow(
				views, &w.viable, w.graph, posToDraw, w.camera, special_joint,
				w.edit_mode, 0, 0, width, height, {} /* TODO w.location.sequence*/, w.style, w.playerDrawer);

			glfwSwapBuffers(window);

			if (w.chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && w.next_pos && w.next_pos->howfar >= 1)
			{
				w.location = w.next_pos->pis;
				w.reorientation = w.next_pos->reorientation;
				w.next_pos = none;
				print_status(w);
			}
		}

		glfwTerminate();

		std::cout << '\n';
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
