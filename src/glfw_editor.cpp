#include "camera.hpp"
#include "editor.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph_util.hpp"
#include <GLFW/glfw3.h>
#include <boost/program_options.hpp>
#include <cmath>

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <numeric>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <stack>

using namespace GrappleMap;

namespace
{
	double whereBetween(V2 const v, V2 const w, V2 const cursor)
	{
		V2 const
			a = cursor - v,
			b = w - v;

		return std::max(0., std::min(1., inner_prod(a, b) / norm2(b) / norm2(b)));
	}

	optional<Reoriented<Location>>
		determineNextPos(
			Graph const & graph, PlayerJoint const j,
			vector<Reoriented<SegmentInSequence>> const & candidates,
			Camera const & camera, V2 const cursor)
	{
		struct Best { Reoriented<Location> loc; double score; };

		optional<Best> best;

		foreach (candidate : candidates)
		{
			V2 const
				v = world2xy(camera, at(from(candidate), j, graph)),
				w = world2xy(camera, at(to(candidate), j, graph));

			double const howfar = whereBetween(v, w, cursor);

			V2 const ultimate = v + (w - v) * howfar;

			double const score = distanceSquared(ultimate, cursor);

			if (!best || score < best->score)
				best = Best{Location{*candidate, howfar}, candidate.reorientation, score};
		}

		if (!best) return boost::none;
		return best->loc;
	}
}

struct Application
{
	explicit Application(boost::program_options::variables_map const & opts, GLFWwindow * w)
		: dbFile(opts["db"].as<string>())
		, editor(loadGraph(dbFile))
		, window(w)
	{
		go_to_desc(opts["start"].as<string>(), editor);
	}

	std::string const dbFile;
	PlayerJoint closest_joint = {{0}, LeftAnkle};
	optional<PlayerJoint> chosen_joint;
	bool edit_mode = false;
	optional<Position> clipboard;
	bool split_view = false;
	Camera camera;
	Editor editor;
	double jiggle = 0;
	double last_cursor_x = 0, last_cursor_y = 0;
	Style style;
	PlayerDrawer playerDrawer;
	PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> candidates;
	GLFWwindow * const window;
};

void print_status(Application const & w)
{
	SegmentInSequence const s = w.editor.getLocation()->segment;
	SeqNum const sn = s.sequence;

	Sequence const & seq = w.editor.getGraph()[sn];

	std::cout
		<< "\r[" << s.segment.index + 1
		<< '/' << seq.positions.size()-1 << "] "
		<< seq.description.front() << string(30, ' ') << std::flush;
}

void translate(Application & w, V3 const v)
{
	w.editor.push_undo();
	w.editor.replace(w.editor.current_position() + v, Graph::NodeModifyPolicy::unintended);
}

void key_callback(GLFWwindow * const glfwWindow, int key, int /*scancode*/, int action, int mods)
{
	Application& w = *reinterpret_cast<Application *>(glfwGetWindowUserPointer(glfwWindow));

	double const translation = mods & GLFW_MOD_SHIFT ? 0.2 : 0.02;

	if (action == GLFW_PRESS)
	{
		if (mods & GLFW_MOD_CONTROL)
			switch (key)
			{
				case GLFW_KEY_Z: w.editor.undo(); return;

				case GLFW_KEY_C: // copy
					w.clipboard = w.editor.current_position(); // todo: store non-reoriented position instead?
					return;

				case GLFW_KEY_V: // paste
					if (w.clipboard) w.editor.replace(*w.clipboard, Graph::NodeModifyPolicy::local);
					return;

				default: return;
			}
		else
			switch (key)
			{
				case GLFW_KEY_X: { swap_players(w.editor); break; }
				case GLFW_KEY_M: { mirror_position(w.editor); break; }
				case GLFW_KEY_P: { w.editor.toggle_playback(); break; }
				case GLFW_KEY_L: { w.editor.toggle_lock(!w.editor.lockedToSelection()); break; }
				// todo: case GLFW_KEY_SPACE: { w.editor.toggle_selected(); break; }
				case GLFW_KEY_INSERT: { w.editor.insert_keyframe(); break; }
				case GLFW_KEY_DELETE: { w.editor.delete_keyframe(); break; }
				case GLFW_KEY_I: w.editor.mirror(); break;

				// set position to center
/*
				case GLFW_KEY_U:
				{
					//push_undo(w);
					Reoriented<SegmentInSequence> loc = segment(w.editor.getLocation());
					Graph const & graph = w.editor.getGraph();
					auto fromPos = from(loc);
					auto toPos = to(loc);
					auto p = between(at(fromPos, graph), at(toPos, graph));
					for(int i = 0; i != 30; ++i) spring(p);
					w.editor.replace(p);
					break;
				}

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
*/
				case GLFW_KEY_KP_4: translate(w, V3{-translation, 0, 0}); break;
				case GLFW_KEY_KP_6: translate(w, V3{translation, 0, 0}); break;
				case GLFW_KEY_KP_8: translate(w, V3{0, 0, -translation}); break;
				case GLFW_KEY_KP_2: translate(w, V3{0, 0, translation}); break;

				case GLFW_KEY_KP_9:
				{
					if (w.editor.playingBack()) return;
					w.editor.push_undo();
					Position p = w.editor.current_position();
					foreach (j : playerJoints) p[j] = yrot(-0.05) * p[j];
					w.editor.replace(p, Graph::NodeModifyPolicy::unintended);
					break;
				}
				case GLFW_KEY_KP_7:
				{
					if (w.editor.playingBack()) return;
					w.editor.push_undo();
					Position p = w.editor.current_position();
					foreach (j : playerJoints) p[j] = yrot(0.05) * p[j];
					w.editor.replace(p, Graph::NodeModifyPolicy::unintended);
					break;
				}

				// new sequence
/*
				case GLFW_KEY_N:
				{
					push_undo(w);
					auto const p = w.graph[w.location];
					w.location.sequence = insert(w.graph, Sequence{{"new"}, {p, p}, none});
					w.location.position.index = 0;
					break;
				}
*/
				case GLFW_KEY_V: flip(w.edit_mode); break;
				case GLFW_KEY_S:
					save(w.editor.getGraph(), w.dbFile);
					break;
				case GLFW_KEY_1: flip(w.split_view); break;
				case GLFW_KEY_B: w.editor.branch(); break;
			}
	}
}

void mouse_button_callback(GLFWwindow * const glfwWindow, int const button, int const action, int /*mods*/)
{
	Application & w = *reinterpret_cast<Application *>(glfwGetWindowUserPointer(glfwWindow));

	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			w.editor.push_undo();

		w.chosen_joint = w.closest_joint;
	}
	else if (action == GLFW_RELEASE)
	{
		w.chosen_joint = none;
		snapToPos(w.editor);
	}
}

void scroll_callback(GLFWwindow * const glfwWindow, double /*xoffset*/, double yoffset)
{
	Application & w = *reinterpret_cast<Application *>(glfwGetWindowUserPointer(glfwWindow));

	if (yoffset == -1) retreat(w.editor);
	else if (yoffset == 1) advance(w.editor);
	else return;

	print_status(w);
}

std::vector<View> const
	single_view
		{ {0, 0, 1, 1, none, 60} },
	split_view
		{ {0, 0, .5, 1, none, 90}
		, {.5, .5, .5, .5, player0, 75}
		, {.5, 0, .5, .5, player1, 75} };

View const * main_view(std::vector<View> const & vv)
{
	foreach (v : vv) if (!v.first_person) return &v;
	return nullptr;
}

void cursor_pos_callback(GLFWwindow * const window, double const xpos, double const ypos)
{
	Application & w = *reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));

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

string const usage =
	"Usage: grapplemap-glfw-editor [OPTIONS] [START]\n";

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
	"  l          - toggle browse lock\n"
	"  p          - toggle playback\n"
	"  space      - add/remove current transition to/from selection\n"
	"  numpad 7/9  - rotate position\n"
	"  numpad arrows - move position\n"
	"  ctrl-c     - copy position to clipboard\n"
	"  ctrl-v     - paste position from clipboard, overwriting current position\n"
	"  ctrl-z     - undo\n"
	"  ins        - duplicate current frame\n"
	"  del        - delete current frame\n"
	"  comma/period - rotate video screen\n"
	"\n"
	"Mouse controls:\n"
	"  scroll wheel              - scroll through frames in sequence\n"
	"  right click + drag joint  - move joint along path\n"
	"  middle click + move       - rotate camera\n"
	"  left click + drag joint   - move joint (only in edit mode)\n";

namespace po = boost::program_options;

boost::optional<po::variables_map> readArgs(int const argc, char const * const * const argv)
{
	po::options_description optdesc("Options");
	optdesc.add_options()
		("help,h", "show this help")
		("start", po::value<string>()->default_value("last-trans"), "see START below")
		("db", po::value<string>()->default_value("GrappleMap.txt"), "database file")
		("video", po::value<string>(), "video MRL (man 1 xine)");

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
		return boost::none;
	}

	return vm;
}

void do_edit(Application & w, V2 const cursor)
{
	Position pos = w.editor.current_position();

	auto const reo = w.editor.getLocation().reorientation;

	V3 dragger = [&]{
			PositionReorientation r;
			r.reorientation.angle = -w.camera.getHorizontalRotation();
			return compose(reo, r)(V3{1,0,0}); // todo: this is wrong, doesn't take swap_players into account
		}();
	V3 const v = pos[*w.chosen_joint];
	V2 const joint_xy = world2xy(w.camera, v);

	double const
		offx = (cursor.x - joint_xy.x)
			/ (world2xy(w.camera, v + dragger).x - joint_xy.x),
		offy = (cursor.y - joint_xy.y)
			* 0.01 / (world2xy(w.camera, v + V3{0,0.01,0}).y - joint_xy.y);

	auto const rj = apply(reo, *w.chosen_joint);

	auto & joint = pos[rj];

	if (reo.mirror) dragger.x = -dragger.x;

	joint.x += dragger.x * offx;
	joint.z += dragger.z * offx;
	joint.y += offy;

	spring(pos, rj);

	w.editor.replace(pos, Graph::NodeModifyPolicy::propagate);
}

void update_camera(Application & w)
{
	if (glfwGetKey(w.window, GLFW_KEY_UP) == GLFW_PRESS) w.camera.rotateVertical(-0.05);
	if (glfwGetKey(w.window, GLFW_KEY_DOWN) == GLFW_PRESS) w.camera.rotateVertical(0.05);
	if (glfwGetKey(w.window, GLFW_KEY_LEFT) == GLFW_PRESS) { w.camera.rotateHorizontal(0.03); w.jiggle = pi(); }
	if (glfwGetKey(w.window, GLFW_KEY_RIGHT) == GLFW_PRESS) { w.camera.rotateHorizontal(-0.03); w.jiggle = 0; }
	if (glfwGetKey(w.window, GLFW_KEY_HOME) == GLFW_PRESS) w.camera.zoom(-0.05);
	if (glfwGetKey(w.window, GLFW_KEY_END) == GLFW_PRESS) w.camera.zoom(0.05);

	if (!w.edit_mode && !w.chosen_joint)
	{
		w.jiggle += 0.01;
		w.camera.rotateHorizontal(sin(w.jiggle) * 0.0005);
	}
}

void do_render(Application const & w)
{
	int width, height;
	glfwGetFramebufferSize(w.window, &width, &height);

	PerPlayerJoint<optional<V3>> colors;
	vector<Viable> viables;
	OrientedPath selection;

	auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

	if (!w.editor.playingBack())
	{
		if (w.edit_mode) foreach (j : playerJoints) colors[j] = white;
		else foreach (j : playerJoints)
			if (!w.candidates[j].empty())
				colors[j] = V3(white) * 0.4 + playerDefs[j.player].color * 0.6;

		colors[special_joint] = yellow;

		viables = determineViables(w.editor.getGraph(),
			from(segment(w.editor.getLocation())),
			special_joint, &w.camera);

		selection = w.editor.getSelection();
	}

	auto & views = w.split_view ? split_view : single_view;

	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);

	renderWindow(views, viables, w.editor.getGraph(), w.editor.current_position(), w.camera, special_joint,
		colors, 0, 0, width, height, selection, w.style, w.playerDrawer,
		[&]{ });
}

double lastTime{};
unique_ptr<Application> app;

void frame()
{
	glfwPollEvents();

	Application & w = *app;

	update_camera(*app);

	int width, height;
	glfwGetFramebufferSize(w.window, &width, &height);

	optional<V2> cursor;

	auto & views = w.split_view ? split_view : single_view;

	if (View const * v = main_view(views))
	{
		w.camera.setViewportSize(v->fov, v->w * width, v->h * height);

		double xpos, ypos;
		glfwGetCursorPos(w.window, &xpos, &ypos);

		double x = xpos / width;
		double y = 1 - (ypos / height);

		if (x >= v->x && x <= v->x + v->w &&
			y >= v->y && y <= v->y + v->h)
			cursor = V2{
				(((x - v->x) / v->w) - 0.5) * 2,
				(((y - v->y) / v->h) - 0.5) * 2};
	}

	OrientedPath const
		tempSel{nonreversed(sequence(w.editor.getLocation()))},
		& sel = w.editor.getSelection().empty() ? tempSel : w.editor.getSelection();
			// ugh

	w.candidates = closeCandidates(
		w.editor.getGraph(), segment(w.editor.getLocation()),
		&w.camera, w.editor.lockedToSelection() ? &sel : nullptr);

	auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

	if (cursor && glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		if (auto best_next_pos = determineNextPos(
				w.editor.getGraph(), special_joint,
				w.candidates[special_joint], w.camera, *cursor))
		{
			w.editor.setLocation(*best_next_pos);
		}

	if (!w.editor.playingBack() && cursor && w.chosen_joint && w.edit_mode
		&& glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		do_edit(w, *cursor);
	else if (cursor && !w.chosen_joint)
	{
		Position const pos = w.editor.current_position();
		w.closest_joint = *minimal(
			playerJoints.begin(), playerJoints.end(),
			[&](PlayerJoint j) { return norm2(world2xy(w.camera, pos[j]) - *cursor); });
	}

	if (!w.edit_mode && !w.chosen_joint && !w.editor.playingBack())
	{
		V3 c = center(w.editor.current_position());
		c.y *= 0.7;
		w.camera.setOffset(c);
	}

	do_render(w);

	glfwSwapBuffers(w.window);

	double const now = glfwGetTime();
	w.editor.frame(now - lastTime);
	lastTime = now;
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		auto const vm = readArgs(argc, argv);
		if (!vm) return 0;

		if (!glfwInit()) throw std::runtime_error("could not initialize GLFW");

		struct GLFWTerminator
		{ ~GLFWTerminator() { glfwTerminate(); } }
		glfwTerminator;

		GLFWwindow * const window = glfwCreateWindow(640, 480, "GrappleMap", nullptr, nullptr);
		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);

		app.reset(new Application(*vm, window));

		glfwSetWindowUserPointer(window, app.get());
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetCursorPosCallback(window, cursor_pos_callback);

		glfwSwapInterval(1);

		lastTime = glfwGetTime();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		while (!glfwWindowShouldClose(window))
		{
			frame();
		}

		std::cout << '\n';
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
