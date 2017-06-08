#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>

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
#include <numeric>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <stack>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace GrappleMap;

size_t makeVertices(); // todo: move

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

GLuint program;
GLint mvp_location;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint LightID, LightEnabledLoc;
GLuint vertex_buffer, vertex_shader, fragment_shader;
GLint vpos_location, vcol_location, norm_location;

vector<BasicVertex> vertices;

struct Application
{
	explicit Application(GLFWwindow * w)
		: window(w)
	{
		style.grid_size = 3;
	}

	PlayerJoint closest_joint = {{0}, LeftAnkle};
	optional<PlayerJoint> chosen_joint;
	bool edit_mode = false;
	optional<Position> clipboard;
	bool split_view = false;
	Camera camera;
	Editor editor{"GrappleMap.txt", "t1383"};
	double jiggle = 0;
	double last_cursor_x = 0, last_cursor_y = 0;
	Style style;
	PlayerDrawer playerDrawer;
	PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> candidates;
	int videoOffset = 0;
	GLFWwindow * const window;
	string joints_to_edit = "single_joint";
	bool confine_horizontal = false;
	bool confine_interpolation = false;
};

void sync_video(Application & app)
{
	/*
	SegmentInSequence const s = app.editor.getLocation()->segment;
	SeqNum const sn = s.sequence;

	Sequence const & seq = app.editor.getGraph()[sn];

	emscripten_run_script((
		"set_slider(" + to_string(s.segment.index)
		+ "," + to_string(seq.positions.size()) + ");").c_str());
	*/
}

void print_status(Application const & w)
{
	SegmentInSequence const s = w.editor.getLocation()->segment;
	SeqNum const sn = s.sequence;

	Sequence const & seq = w.editor.getGraph()[sn];

	std::cout
		<< "\r[" << s.segment.index + 1
		<< '/' << seq.positions.size()-1 << "] "
		<< seq.description.front() << string(30, ' ') << std::flush;

	emscripten_run_script((
		"set_slider(" + to_string(s.segment.index)
		+ "," + to_string(seq.positions.size()) + ");").c_str());
}

void translate(Application & w, V3 const v)
{
	w.editor.push_undo();
	w.editor.replace(w.editor.current_position() + v);
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
					if (w.clipboard) w.editor.replace(*w.clipboard);
					return;

				case GLFW_KEY_KP_ADD: { w.videoOffset += 10; sync_video(w); break; }
				case GLFW_KEY_KP_SUBTRACT: { w.videoOffset -= 10; sync_video(w); break; }

				default: return;
			}
		else
			switch (key)
			{
				case GLFW_KEY_X: { swap_players(w.editor); break; }
				case GLFW_KEY_M: { mirror_position(w.editor); break; }
				case GLFW_KEY_P: { w.editor.toggle_playback(); sync_video(w); break; }
				case GLFW_KEY_L: { w.editor.toggle_lock(!w.editor.lockedToSelection()); break; }
				case GLFW_KEY_INSERT: { w.editor.insert_keyframe(); break; }
				case GLFW_KEY_DELETE: { w.editor.delete_keyframe(); break; }
				case GLFW_KEY_I: w.editor.mirror(); break;


				// set position to center
/*
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
					w.editor.replace(p);
					break;
				}
				case GLFW_KEY_KP_7:
				{
					if (w.editor.playingBack()) return;
					w.editor.push_undo();
					Position p = w.editor.current_position();
					foreach (j : playerJoints) p[j] = yrot(0.05) * p[j];
					w.editor.replace(p);
					break;
				}

				case GLFW_KEY_KP_ADD: { ++w.videoOffset; sync_video(w); break; }
				case GLFW_KEY_KP_SUBTRACT: { --w.videoOffset; sync_video(w); break; }

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
				case GLFW_KEY_S: w.editor.save(); break;
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

void gui_highlight_segment(Location const & loc)
{
	std::ostringstream script;
	script << "highlight_segment(" << loc.segment.sequence.index << ',' << loc.segment.segment << ',';

	if (auto pos = position(loc))
		script << int(pos->position.index);
	else
		script << "-1";
	
	script << ");";

	emscripten_run_script(script.str().c_str());
}

void scroll_callback(GLFWwindow * const glfwWindow, double /*xoffset*/, double yoffset)
{
	Application & w = *reinterpret_cast<Application *>(glfwGetWindowUserPointer(glfwWindow));

	if (yoffset > 0) retreat(w.editor);
	else if (yoffset < 0) advance(w.editor);
	else return;

	gui_highlight_segment(*w.editor.getLocation());
}

unique_ptr<Application> app;

void update_selection_gui()
{
	Graph const & g = app->editor.getGraph();
 	auto const & selection = app->editor.getSelection();

	std::ostringstream script;
	script << "set_selection([";
	foreach (seq : selection)
	{
		tojs(**seq, g, script);
		script << ',';
	}
	script << "],[";

	if (!selection.empty())
	{
		Reoriented<Reversible<SeqNum>> x = selection.back();
		Reoriented<NodeNum> n = to(x, g);

		foreach (o : g[*n].out)
		{
			script << '[' << o->index << ',';
			tojs(g[*o].description, script);
			script << "],";
		}
	}

	script << "],[";

	if (!selection.empty())
	{
		Reoriented<Reversible<SeqNum>> x = selection.front();
		Reoriented<NodeNum> n = from(x, g);

		foreach (i : g[*n].in)
		{
			script << '[' << i->index << ',';
			tojs(g[*i].description, script);
			script << "],";
		}
	}

	script << "]);";
	emscripten_run_script(script.str().c_str());

	gui_highlight_segment(*app->editor.getLocation());
}

extern "C" // called from javascript
{
	void loadDB(char const * s)
	{
		std::istringstream iss(s);
		Graph g = loadGraph(iss);

		app->editor.load(std::move(g));

		foreach (c : app->candidates.values)
			foreach (x : c) x.clear();

		app->chosen_joint = none;
		update_selection_gui();
	}

	void gui_command(char const * s)
	{
		std::istringstream iss(s);

		string cmd;
		iss >> cmd;

		std::istream_iterator<string> i(iss), e;
		vector<string> args(i, e);

		if (cmd == "mode")
		{
			set_playing(app->editor, args[0] == "playback");
			app->edit_mode = (args[0] == "edit");
		}
		else if (cmd == "mirror_view") app->editor.mirror();
		else if (cmd == "insert_keyframe")
		{
			app->editor.insert_keyframe();
			update_selection_gui();
		}
		else if (cmd == "delete_keyframe")
		{
			app->editor.delete_keyframe();
			update_selection_gui();
		}
		else if (cmd == "swap_players") swap_players(app->editor);
		else if (cmd == "confine") app->editor.toggle_lock(args[0] == "true");
		else if (cmd == "confine_interpolation") app->confine_interpolation = (args[0] == "true");
		else if (cmd == "confine_horizontal") app->confine_horizontal = (args[0] == "true");
		else if (cmd == "joints_to_edit") app->joints_to_edit = args[0];
		else if (cmd == "goto_segment")
			app->editor.go_to(SegmentInSequence
				{ SeqNum{uint32_t(stol(args[0]))}
				, SegmentNum{uint8_t(stol(args[1]))} });
		else if (cmd == "goto_position")
			app->editor.go_to(PositionInSequence
				{ SeqNum{uint32_t(stol(args[0]))}
				, PosNum{uint8_t(stol(args[1]))} });
		else if (cmd == "set_selected")
		{
			app->editor.set_selected(SeqNum{uint32_t(stol(args[0]))}, args[1] == "true");
			update_selection_gui();
		}
		else if (cmd == "resolution")
		{
			int const
				w = std::stoi(args[0]),
				h = std::stoi(args[1]);

			if (w > 128 && h > 128)
				glfwSetWindowSize(app->window, w, h);
		}
		else
		{
			std::cout << s << "!?\n";
		}
	}
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

void do_edit(Application & w, V2 const cursor)
{
	Position pos = w.editor.current_position();

	auto const reo = w.editor.getLocation().reorientation;

	V3 hdragger = [&]{
			PositionReorientation r;
			r.reorientation.angle = -w.camera.getHorizontalRotation();
			return compose(reo, r)(V3{1,0,0}); // todo: this is wrong, doesn't take swap_players into account
		}();
	V3 vdragger = [&]{
			PositionReorientation r;
			r.reorientation.angle = -w.camera.getHorizontalRotation();
			return compose(reo, r)(V3{0,0,1});
		}();
	V3 const v = pos[*w.chosen_joint];
	V2 const joint_xy = world2xy(w.camera, v);

	double const
		offh = (cursor.x - joint_xy.x)
			/ (world2xy(w.camera, v + hdragger).x - joint_xy.x),
		offv = (cursor.y - joint_xy.y)
			/ (world2xy(w.camera, v + vdragger).y - joint_xy.y),
		offy = (cursor.y - joint_xy.y)
			* 0.01 / (world2xy(w.camera, v + V3{0,0.01,0}).y - joint_xy.y);

	if (reo.mirror) hdragger.x = -hdragger.x;

	auto drag = [&](PlayerJoint j)
		{
			auto & joint = pos[j];

			if (w.confine_horizontal)
			{
				joint.x += hdragger.x * offh + vdragger.x * offv;
				joint.z += hdragger.z * offh + vdragger.z * offv;
			}
			else
			{
				joint.x += hdragger.x * offh;
				joint.z += hdragger.z * offh;
				joint.y += offy;
			}
		};

	if (w.joints_to_edit == "single_joint")
	{
		PlayerJoint const rj = apply(reo, *w.chosen_joint);
		drag(rj);
		spring(pos, rj);
	}
	else
	{
		foreach(rj : playerJoints)
		{
			if (w.joints_to_edit == "whole_player" && rj.player != w.chosen_joint->player)
				continue;

			drag(rj);
		}

		spring(pos);
	}

	w.editor.replace(pos);
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

size_t do_render(Application const & w, vector<BasicVertex> & out)
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
	
	return renderWindow(views, viables, w.editor.getGraph(), w.editor.current_position(), w.camera, special_joint,
		colors, 0, 0, width, height, selection, w.editor.getLocation()->segment, w.style, w.playerDrawer, []{}, out);
}

double lastTime{};

glm::mat4 to_glm(M m)
{
	return glm::mat4(
		m[ 0],m[ 1],m[ 2],m[ 3],
		m[ 4],m[ 5],m[ 6],m[ 7],
		m[ 8],m[ 9],m[10],m[11],
		m[12],m[13],m[14],m[15]);
}

void frame()
{
	glfwPollEvents();

	Application & w = *app;

	update_camera(*app);

	int width, height;
	glfwGetFramebufferSize(w.window, &width, &height);

	// todo: this is all wrong
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		if (auto best_next_loc = determineNextPos(
				w.editor.getGraph(), special_joint,
				w.candidates[special_joint], w.camera, *cursor))
		{
			SegmentInSequence const newseg = (*best_next_loc)->segment;

			if (w.editor.getLocation()->segment != newseg)
				gui_highlight_segment(**best_next_loc);

			w.editor.setLocation(*best_next_loc);
			sync_video(w);
		}

	if (w.editor.playingBack()) sync_video(w);

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

	size_t non_hud_elems = makeVertices();

	glm::mat4
		ProjectionMatrix = to_glm(w.camera.projection()),
		ViewMatrix = to_glm(w.camera.model_view()),
		ModelMatrix = glm::mat4(1.0),
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

	glm::vec3 lightPos = glm::vec3(4,4,4);
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

	glUniform1f(LightEnabledLoc, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, non_hud_elems);

	if (vertices.size() != non_hud_elems)
	{
		glDisable(GL_DEPTH_TEST);

		glUniform1f(LightEnabledLoc, 0.0);
		glDrawArrays(GL_TRIANGLES, non_hud_elems, vertices.size() - non_hud_elems);

		glEnable(GL_DEPTH_TEST);
	}

	glfwSwapBuffers(w.window);

	double const now = glfwGetTime();

	if (auto loc_before = w.editor.playingBack())
	{
		w.editor.frame(now - lastTime);

		auto const loc_now = **w.editor.playingBack();

		if ((*loc_before)->segment != loc_now.segment) gui_highlight_segment(loc_now);
	}

	lastTime = now;
}

void output_error(int error, const char * msg)
{
    fprintf(stderr, "Error: %s\n", msg);
}

int check_compiled(int shader)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if(success == GL_FALSE)
	{
		GLint max_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

		GLchar err_log[max_len];
		glGetShaderInfoLog(shader, max_len, &max_len, &err_log[0]);
		glDeleteShader(shader);

		fprintf(stderr, "Shader compilation failed: %s\n", err_log);
	}

	return success;
}

int check_linked(int program)
{
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if(success == GL_FALSE)
	{
		GLint max_len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

		GLchar err_log[max_len];
		glGetProgramInfoLog(program, max_len, &max_len, &err_log[0]);

		fprintf(stderr, "Program linking failed: %s\n", err_log);
	}

	return success;
}

std::string readFile(std::string path)
{
	std::ifstream f(path);
	std::istreambuf_iterator<char> i(f), e;
	return std::string(i, e);
}

glm::vec3 to_glm(V3 v) { return {v.x, v.y, v.z}; }

size_t makeVertices()
{
	vertices.clear();

	size_t r = do_render(*app, vertices);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(BasicVertex), vertices.data(), GL_DYNAMIC_DRAW);

	return r;
}

void fatal_error(std::string const & msg)
{
	fputs(msg.c_str(), stderr);
	emscripten_force_exit(EXIT_FAILURE);
}

int main()
{
	glfwSetErrorCallback(output_error);

	if (!glfwInit()) fatal_error("glfwInit");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow * window = glfwCreateWindow(1024, 768, "My Title", NULL, NULL);

	if (!window) fatal_error("glfwCreateWindow");

	glfwMakeContextCurrent(window);

	lastTime = glfwGetTime();

	app.reset(new Application(window));

	glfwSetWindowUserPointer(window, app.get());
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	glClearColor(0.f, 0.f, 0.f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 

//	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &vertex_buffer);

	makeVertices();

	std::string const
		vertex_shader_src = readFile("triangle.vertexshader"),
		fragment_shader_src = readFile("triangle.fragmentshader");
	char const * vertex_shader_text = vertex_shader_src.c_str();
	char const * fragment_shader_text = fragment_shader_src.c_str();

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);
	check_compiled(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);
	check_compiled(fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	check_linked(program);

	vpos_location = glGetAttribLocation(program, "vertexPosition_modelspace");
	norm_location = glGetAttribLocation(program, "vertexNormal_modelspace");
	vcol_location = glGetAttribLocation(program, "vertexColor");

	mvp_location = glGetUniformLocation(program, "MVP");
	ViewMatrixID = glGetUniformLocation(program, "V");
	ModelMatrixID = glGetUniformLocation(program, "M");
	LightID = glGetUniformLocation(program, "LightPosition_worldspace");
	LightEnabledLoc = glGetUniformLocation(program, "LightEnabled");

	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
		sizeof(float) * 10, (void*) 0);

	glEnableVertexAttribArray(norm_location);
	glVertexAttribPointer(norm_location, 3, GL_FLOAT, GL_FALSE,
		sizeof(float) * 10, (void*) (sizeof(float) * 3));

	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 4, GL_FLOAT, GL_FALSE,
		sizeof(float) * 10, (void*) (sizeof(float) * 6));

	glUseProgram(program);

	update_selection_gui();

	emscripten_set_main_loop(frame, 0, 0);
}
