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
				if (std::abs(int(other.position) - int(from.position)) != 1)
					continue;
			}
			else
			{
				if (auto const current_node = node(graph, from))
				{
					auto const & e_to = graph.to(other.sequence);
					auto const & e_from = graph.from(other.sequence);

					if (!(current_node->node == e_to.node
							&& other.position == last_pos(graph, other.sequence) - 1) &&
						!(current_node->node == e_from.node
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
	explicit Window(string const f)
		: filename(f), graph(loadGraph(filename))
	{}

	string filename;
	Graph graph;
	PositionInSequence location{{0}, 0};
	PlayerJoint closest_joint = {0, LeftAnkle};
	optional<PlayerJoint> chosen_joint;
	bool edit_mode = false;
	optional<Position> clipboard;
	Camera camera;
	bool split_view = false;
	double jiggle = 0;
	Viables viable;
	PositionReorientation reorientation{};
	optional<NextPosition> next_pos;
	std::stack<std::pair<Graph, PositionInSequence>> undo;

	std::map<NodeNum, unsigned> anim_next;
		// the unsigned is an index into out(graph, n)
};

void print_status(Window const & w)
{
	auto && seq = w.graph[w.location.sequence];

	std::cout
		<< "\r[" << w.location.position + 1
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

					vector<SeqNum> const v = out(w.graph, n);
					if (v.empty()) return;
					unsigned & an = w.anim_next[n];
					an = (an == 0 ? v.size() - 1 : an - 1);
					return;
				}

				case GLFW_KEY_DOWN:
				{
					NodeNum const n = w.graph.from(w.location.sequence).node;

					vector<SeqNum> const v = out(w.graph, n);
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
					if (w.location.sequence.index != 0)
					{
						--w.location.sequence.index;
						w.location.position = 0;
						w.reorientation = PositionReorientation();
						print_status(w);
					}
					break;

				case GLFW_KEY_PAGE_DOWN:
					if (w.location.sequence.index != w.graph.num_sequences() - 1)
					{
						++w.location.sequence.index;
						w.location.position = 0;
						w.reorientation = PositionReorientation();
						print_status(w);
					}
					break;

				// swap players

				case GLFW_KEY_X:
				{
					push_undo(w);
					auto p = w.graph[w.location];
					swap(p[0], p[1]);
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
						auto const j = apply(w.reorientation, w.closest_joint); // todo: also for KEY_J
						replace(w.graph, w.location, j, w.graph[*p][j], false);
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
						w.graph.replace(w.location, p, false);
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
					w.graph.replace(w.location, p, true);
					break;
				}
				case GLFW_KEY_KP_7:
				{
					push_undo(w);
					auto p = w.graph[w.location];
					foreach (j : playerJoints) p[j] = xyz(yrot(0.05) * V4(p[j], 1));
					w.graph.replace(w.location, p, true);
					break;
				}

				// new sequence

				case GLFW_KEY_N:
				{
					push_undo(w);
					auto const p = w.graph[w.location];
					w.location.sequence = insert(w.graph, Sequence{{"new"}, {p, p}});
					w.location.position = 0;
					break;
				}
				case GLFW_KEY_V: w.edit_mode = !w.edit_mode; break;

				case GLFW_KEY_S: save(w.graph, w.filename); break;

				case GLFW_KEY_1: w.split_view = !w.split_view; break;

				case GLFW_KEY_B: // branch
				{
					push_undo(w);
					split_at(w.graph, w.location);
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
		if (w.location.position != 0)
		{
			--w.location.position;
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
		, {.5, .5, .5, .5, optional<unsigned>(0), 75}
		, {.5, 0, .5, .5, optional<unsigned>(1), 75} };

View const * main_view(std::vector<View> const & vv)
{
	foreach (v : vv) if (!v.first_person) return &v;
	return nullptr;
}

void backward(Window & w)
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
				auto const v = out(w.graph, n);

				if (!v.empty())
				{
					PositionReorientation r;

					SeqNum const next_seq = v[w.anim_next[n]];

					Position const before = w.reorientation(w.graph[w.location]);
					Position const after = r(w.graph[first_pos_in(next_seq)]);

					assert(basicallySame(
						w.reorientation(w.graph[w.location]),
						r(w.graph[first_pos_in(next_seq)])));

					w.next_pos = NextPosition{{next_seq, 1}, hf, r};
				}
			}
			else w.next_pos = NextPosition{{w.location.sequence, w.location.position+1}, hf, {}};

			print_status(w);
		}
	}
}

bool all_digits(string const & s)
{
	return all_of(s.begin(), s.end(), [](char c){return std::isdigit(c);});
}

int main(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	try
	{
		po::options_description desc("options");
		desc.add_options()
			("help", "show this help")
			("start", po::value<string>(), "initial node (by number or first line of description)")
			("db", po::value<string>()->default_value("positions.txt"), "position database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { std::cout << desc << '\n'; return 0; }

		Window w(vm["db"].as<std::string>());

		string const start_str = vm["start"].as<string>();

		NodeNum start_node;
		
		if (all_digits(start_str))
			start_node.index = std::stol(start_str);
		else if (auto o = node_by_desc(w.graph, start_str))
			start_node = *o;
		else if (auto o = seq_by_desc(w.graph, start_str))
			start_node = w.graph.from(*o).node; // todo: set seq directly, because node_as_posinseq below may pick another seq
		else throw std::runtime_error("no such node");

		if (auto pis = node_as_posinseq(w.graph, start_node))
			w.location = *pis;
		else
			throw std::runtime_error("cannot find sequence starting/ending at specified node");

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
					w.viable[j] = determineViables(w.graph, w.location, j, w.edit_mode, w.camera, w.reorientation);

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

				V4 dragger = yrot(-w.camera.getHorizontalRotation() - w.reorientation.reorientation.angle) * V4{{1,0,0},0};
				if (w.reorientation.mirror) dragger.x = -dragger.x;
				
				V2 const off = *cursor - world2xy(w.camera, apply(w.reorientation, new_pos, *w.chosen_joint));

				auto & joint = new_pos[apply(w.reorientation, *w.chosen_joint)];

				joint.x += dragger.x * off.x;
				joint.z += dragger.z * off.x;
				joint.y = std::max(jointDefs[w.chosen_joint->joint].radius, joint.y + off.y);

				spring(new_pos, w.chosen_joint);

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

			auto const center = xz(posToDraw[0][Core] + posToDraw[1][Core]) / 2;

			w.camera.setOffset(center);

			auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

			glfwGetFramebufferSize(window, &width, &height);
			renderWindow(
				views, &w.viable, w.graph, window, posToDraw, w.camera, special_joint, w.edit_mode, width, height, w.location.sequence);

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
