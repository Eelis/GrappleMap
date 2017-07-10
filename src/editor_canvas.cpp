#include "editor_canvas.hpp"

namespace GrappleMap
{
	EditorCanvas * editor_canvas;

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
					best = Best{{Location{*candidate, howfar}, candidate.reorientation}, score};
			}

			if (!best) return boost::none;
			return best->loc;
		}

		vector<View> const
			external_view
				{ {0, 0, 1, 1, none, 60} },
			player0_view
				{ {0, 0, 1, 1, player0, 60} },
			player1_view
				{ {0, 0, 1, 1, player1, 60} };

		bool operator!=(Dirty const & a, Dirty const & b)
		{
			return a.nodes_added != b.nodes_added
				|| a.nodes_changed != b.nodes_changed
				|| a.edges_added != b.edges_added
				|| a.edges_changed != b.edges_changed;
		}

		using HighlightableLoc = pair<SegmentInSequence, optional<PositionInSequence>>;

		HighlightableLoc highlightable_loc(Location const & loc)
		{
			return {loc.segment, position(loc)};
		}

		void gui_highlight_segment(HighlightableLoc const loc)
		{
			EM_ASM_({ highlight_segment($0, $1, $2); }
				, loc.first.sequence.index
				, loc.first.segment.index
				, (loc.second ? int(loc.second->position.index) : -1));
		}

		void mouse_button_callback(GLFWwindow * const glfwWindow, int const button, int const action, int /*mods*/)
		{
			EditorCanvas & w = *reinterpret_cast<EditorCanvas *>(glfwGetWindowUserPointer(glfwWindow));

			if (action == GLFW_PRESS)
			{
				if (button == GLFW_MOUSE_BUTTON_LEFT)
					w.editor.push_undo();

				w.chosen_joint = w.closest_joint;
			}
			else if (action == GLFW_RELEASE)
			{
				w.chosen_joint = none;

				if (snapToPos(w.editor))
					gui_highlight_segment(highlightable_loc(*w.editor.getLocation()));
			}
		}

		void update_modified(EditorCanvas & app, bool const force = false)
		{
			Graph const & g = app.editor.getGraph();

			Dirty d;
			d.nodes_added.reserve(app.dirty.nodes_added.size() + 5);
			d.edges_added.reserve(app.dirty.edges_added.size() + 5);
			d.nodes_changed.reserve(app.dirty.nodes_changed.size() + 5);
			d.edges_changed.reserve(app.dirty.edges_changed.size() + 5);

			foreach(n : nodenums(g))
				switch (g[n].modified)
				{
					case modified: d.nodes_changed.push_back(n); break;
					case added: d.nodes_added.push_back(n); break;
					default: break;
				}

			foreach(s : seqnums(g))
				switch (g[s].modified)
				{
					case modified: d.edges_changed.push_back(s); break;
					case added: d.edges_added.push_back(s); break;
					default: break;
				}

			if (force || app.dirty != d)
			{
				app.dirty = move(d);
				EM_ASM({ update_modified(); });
			}
		}

		void fatal_error(std::string const & msg)
		{
			fputs(msg.c_str(), stderr);
			emscripten_force_exit(EXIT_FAILURE);
		}

		void output_error(int /*error*/, const char * msg)
		{
			fprintf(stderr, "Error: %s\n", msg);
		}

		View const * main_view(std::vector<View> const & vv)
		{
			foreach (v : vv) if (!v.first_person) return &v;
			return nullptr;
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

		void update_camera(EditorCanvas & w)
		{
			if (!w.ignore_keyboard)
			{
				if (glfwGetKey(w.window, GLFW_KEY_UP) == GLFW_PRESS) w.camera.rotateVertical(-0.05);
				if (glfwGetKey(w.window, GLFW_KEY_DOWN) == GLFW_PRESS) w.camera.rotateVertical(0.05);
				if (glfwGetKey(w.window, GLFW_KEY_LEFT) == GLFW_PRESS) { w.camera.rotateHorizontal(0.03); w.jiggle = pi(); }
				if (glfwGetKey(w.window, GLFW_KEY_RIGHT) == GLFW_PRESS) { w.camera.rotateHorizontal(-0.03); w.jiggle = 0; }
				if (glfwGetKey(w.window, GLFW_KEY_HOME) == GLFW_PRESS) w.camera.zoom(-0.05);
				if (glfwGetKey(w.window, GLFW_KEY_END) == GLFW_PRESS) w.camera.zoom(0.05);
			}

			if (!w.edit_mode && !w.chosen_joint)
			{
				w.jiggle += 0.01;
				w.camera.rotateHorizontal(sin(w.jiggle) * 0.0005);
			}
		}

		void do_edit(EditorCanvas & app, V2 const cursor)
		{
			Position pos = app.editor.current_position();

			if (app.confine_interpolation)
			{
				Graph const & graph = app.editor.getGraph();

				if (auto p = position(app.editor.getLocation()))
				if (auto prev_pos = prev(*p))
				if (auto next_pos = next(*p, graph))
				{
					Position const
						prevv = at(*prev_pos, graph),
						nextv = at(*next_pos, graph);
				
					double const howfar = whereBetween(
						world2xy(app.camera, prevv[*app.chosen_joint]),
						world2xy(app.camera, nextv[*app.chosen_joint]),
						cursor);

					auto drag = [&](PlayerJoint j)
						{
							pos[j] = prevv[j] + (nextv[j] - prevv[j]) * howfar;
						};

					if (app.joints_to_edit == "single_joint")
					{
						drag(*app.chosen_joint);
						spring(pos, *app.chosen_joint);
					}
					else
					{
						foreach(rj : playerJoints)
						{
							if (app.joints_to_edit == "whole_player" && rj.player != app.chosen_joint->player)
								continue;

							drag(rj);
						}

						spring(pos);
					}
				}
			}
			else
			{
				V3 hdragger = [&]{
						PositionReorientation r;
						r.reorientation.angle = -app.camera.getHorizontalRotation();
						return r(V3{1,0,0});
					}();
				V3 vdragger = [&]{
						PositionReorientation r;
						r.reorientation.angle = -app.camera.getHorizontalRotation();
						return r(V3{0,0,1});
					}();
					// todo: rewrite these sensibly

				V3 const v = pos[*app.chosen_joint];
				V2 const joint_xy = world2xy(app.camera, v);

				double const
					offh = (cursor.x - joint_xy.x)
						/ (world2xy(app.camera, v + hdragger).x - joint_xy.x),
					offv = (cursor.y - joint_xy.y)
						/ (world2xy(app.camera, v + vdragger).y - joint_xy.y),
					offy = (cursor.y - joint_xy.y)
						* 0.01 / (world2xy(app.camera, v + V3{0,0.01,0}).y - joint_xy.y);

				auto drag = [&](PlayerJoint j)
					{
						auto & joint = pos[j];

						if (app.confine_horizontal)
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

				if (app.joints_to_edit == "single_joint")
				{
					drag(*app.chosen_joint);
					spring(pos, *app.chosen_joint);
				}
				else
				{
					foreach(rj : playerJoints)
					{
						if (app.joints_to_edit == "whole_player" && rj.player != app.chosen_joint->player)
							continue;

						drag(rj);
					}

					spring(pos);
				}
			}

			if (!node(app.editor.getGraph(), *app.editor.getLocation()))
				app.editor.replace(pos, Graph::NodeModifyPolicy::unintended);
			else if (!app.confine_local_edits)
				app.editor.replace(pos, Graph::NodeModifyPolicy::propagate);
			else if (app.joints_to_edit == "both_players" && app.confine_horizontal)
				app.editor.replace(pos, Graph::NodeModifyPolicy::unintended);
			else
			{
				std::cerr << "Ignoring edit because unless non-local editing is enabled, "
					"only rotations and horizontally constrained translations to both "
					"players simultaneously are allowed for connecting nodes." << std::endl;
				return;
			}

			update_modified(app);
		}

		glm::mat4 to_glm(M m)
		{
			return glm::mat4(
				m[ 0],m[ 1],m[ 2],m[ 3],
				m[ 4],m[ 5],m[ 6],m[ 7],
				m[ 8],m[ 9],m[10],m[11],
				m[12],m[13],m[14],m[15]);
		}

		void scroll_callback(GLFWwindow * const glfwWindow, double /*xoffset*/, double yoffset)
		{
			EditorCanvas & w = *reinterpret_cast<EditorCanvas *>(glfwGetWindowUserPointer(glfwWindow));

			if (yoffset > 0) retreat(w.editor);
			else if (yoffset < 0) advance(w.editor);
			else return;

			gui_highlight_segment(highlightable_loc(*w.editor.getLocation()));
		}

		void update_selection_gui(EditorCanvas const & w)
		{
			EM_ASM({ update_selection(); });
			gui_highlight_segment(highlightable_loc(*w.editor.getLocation()));
		}

		void ensure_nonempty_selection(Editor & editor)
		{
			if (editor.getSelection().empty())
				editor.set_selected(
					editor.getLocation()->segment.sequence, true);

			editor.toggle_lock(true);
		}

		void cursor_pos_callback(GLFWwindow * const window, double const xpos, double const ypos)
		{
			EditorCanvas & w = *reinterpret_cast<EditorCanvas *>(glfwGetWindowUserPointer(window));

			int width, height;
			glfwGetFramebufferSize(w.window, &width, &height);

			auto & views = w.views();

			View const * v = main_view(views);
			if (!v) return;
			
			double const
				x = xpos / width,
				y = 1 - (ypos / height);

			if (!(x >= v->x && x <= v->x + v->w &&
				  y >= v->y && y <= v->y + v->h)) return;

			V2 const newcur{
				(((x - v->x) / v->w) - 0.5) * 2,
				(((y - v->y) / v->h) - 0.5) * 2};

			if (w.cursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS)
			{
				double const speed = 1;

				w.camera.rotateVertical((newcur.y - w.cursor->y) * speed);
				w.camera.rotateHorizontal((newcur.x - w.cursor->x) * speed);
			}

			if (w.cursor && w.edit_mode && w.transform_rotate && !w.editor.playingBack()
				&& glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				auto const mat = yrot(newcur.x - w.cursor->x);

				Position p = w.editor.current_position();

				foreach (j : playerJoints)
				{
					if (w.joints_to_edit == "whole_player" && j.player != w.chosen_joint->player)
						continue;
					
					p[j] = mat * p[j];
				}

				if (!node(w.editor.getGraph(), *w.editor.getLocation()))
					w.editor.replace(p, Graph::NodeModifyPolicy::unintended);
				else if (!w.confine_local_edits)
					w.editor.replace(p, Graph::NodeModifyPolicy::propagate);
				else if (w.joints_to_edit == "both_players")
					w.editor.replace(p, Graph::NodeModifyPolicy::unintended);

				update_modified(w);
			}

			w.cursor = newcur;

			if (!w.editor.playingBack() &&
				glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS &&
				glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_3) != GLFW_PRESS &&
				!w.chosen_joint)
			{
				Position const pos = w.editor.current_position();
				w.closest_joint = *minimal(
					playerJoints.begin(), playerJoints.end(),
					[&](PlayerJoint j) { return norm2(world2xy(w.camera, pos[j]) - newcur); });
			}
		}

		void cursor_enter_callback(GLFWwindow * const window, int const entered)
		{
			EditorCanvas & w = *reinterpret_cast<EditorCanvas *>(glfwGetWindowUserPointer(window));

			if (!entered) w.cursor = boost::none;
		}

		size_t do_render(EditorCanvas const & w, vector<BasicVertex> & out)
		{
			int width, height;
			glfwGetFramebufferSize(w.window, &width, &height);

			PerPlayerJoint<optional<V3>> colors;
			vector<Viable> viables;
			OrientedPath selection;

			auto const special_joint = w.chosen_joint ? *w.chosen_joint : w.closest_joint;

			if (!w.renderPlain())
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

			return renderWindow(w.views(), viables, w.editor.getGraph(), w.displayPos(), w.camera, special_joint,
				colors, 0, 0, width, height, selection, w.editor.getLocation()->segment, w.style, w.playerDrawer, []{}, out);
		}

		void do_frame() { editor_canvas->frame(); }
	}

	size_t EditorCanvas::makeVertices()
	{
		vertices.clear();

		size_t r = do_render(*this, vertices);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(BasicVertex), vertices.data(), GL_DYNAMIC_DRAW);

		return r;
	}

	Position EditorCanvas::displayPos() const
	{
		return editor.current_position();
	}

	void EditorCanvas::frame()
	{
		glfwPollEvents();

		update_camera(*this);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// todo: this is all wrong
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (View const * v = main_view(views()))
			camera.setViewportSize(v->fov, v->w * width, v->h * height);
				// todo: only do when viewport size actually changes?

		OrientedPath const
			tempSel{nonreversed(sequence(editor.getLocation()))},
			& sel = editor.getSelection().empty() ? tempSel : editor.getSelection();
				// ugh

		candidates = closeCandidates(
			editor.getGraph(), segment(editor.getLocation()),
			&camera, editor.lockedToSelection() ? &sel : nullptr);

		auto const special_joint = chosen_joint ? *chosen_joint : closest_joint;

		if (cursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
			if (auto best_next_loc = determineNextPos(
					editor.getGraph(), special_joint,
					candidates[special_joint], camera, *cursor))
			{
				HighlightableLoc const newhl = highlightable_loc(**best_next_loc);

				if (highlightable_loc(*editor.getLocation()) != newhl)
					gui_highlight_segment(newhl);

				editor.setLocation(*best_next_loc);
			}

		if (!editor.playingBack() && cursor && chosen_joint && edit_mode
			&& !transform_rotate
			&& glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			do_edit(*this, *cursor);

		if (!chosen_joint && !editor.playingBack())
		{
			V3 c = center(editor.current_position());
			c.y *= 0.7;
			camera.setOffset(c);
		}

		size_t non_hud_elems = makeVertices();

		glm::mat4
			ProjectionMatrix = to_glm(camera.projection()),
			ViewMatrix = to_glm(camera.model_view()),
			ModelMatrix = glm::mat4(1.0),
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glUniform1f(LightEnabledLoc, 1.0);
		glDrawArrays(GL_TRIANGLES, 0, non_hud_elems);

		if (vertices.size() != non_hud_elems)
		{
			glDisable(GL_DEPTH_TEST);

			glUniform1f(LightEnabledLoc, 0.0);
			glDrawArrays(GL_TRIANGLES, non_hud_elems, vertices.size() - non_hud_elems);

			glEnable(GL_DEPTH_TEST);
		}

		glfwSwapBuffers(window);

		double const
			now = glfwGetTime(),
			elapsed = now - lastTime;

		progress(elapsed);

		lastTime = now;
	}

	EditorCanvas::EditorCanvas()
	{
		glfwSetErrorCallback(output_error);

		if (!glfwInit()) fatal_error("glfwInit");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		window = glfwCreateWindow(1024, 768, "My Title", NULL, NULL);

		if (!window) fatal_error("glfwCreateWindow");

		lastTime = glfwGetTime();

		glfwMakeContextCurrent(window);

		style.grid_size = 4;
		style.grid_color = V3{.25, .25, .25};

		Location l;
		l.segment.sequence = SeqNum{1383};
		editor.setLocation({l, {}});
		editor.set_selected(l.segment.sequence, true);
		editor.toggle_lock(true);

		glfwSetWindowUserPointer(window, this);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetCursorPosCallback(window, cursor_pos_callback);
		glfwSetCursorEnterCallback(window, cursor_enter_callback);

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

		emscripten_set_main_loop(do_frame, 0, 0);
	}

	void EditorCanvas::progress(double const elapsed)
	{
		if (auto loc_before = editor.playingBack())
		{
			editor.frame(elapsed);

			auto const loc_now = highlightable_loc(**editor.playingBack());

			if (highlightable_loc(**loc_before) != loc_now)
				gui_highlight_segment(loc_now);
		}
	}

	vector<View> const & EditorCanvas::views() const
	{
		return external_view; // player0_view;
	}

	unique_ptr<EditorCanvas> web_editor;

	EMSCRIPTEN_BINDINGS(GrappleMap_editor_canvas)
	{
		emscripten::function("editor_main", +[]
			{
				web_editor.reset(new EditorCanvas);
				editor_canvas = web_editor.get();
				update_selection_gui(*editor_canvas);
			});

		emscripten::function("loadDB", +[](std::string const & s)
		{
			std::istringstream iss(s);

			EditorCanvas & app = *editor_canvas;

			app.editor = Editor(loadGraph(iss));
			app.editor.set_selected(app.editor.getLocation()->segment.sequence, true);
			app.editor.toggle_lock(true);

			foreach (c : app.candidates.values)
				foreach (x : c) x.clear();

			app.chosen_joint = none;
			update_selection_gui(app);
		});

		emscripten::function("getDB", +[]
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

			std::ostringstream oss;
			save(editor_canvas->editor.getGraph(), oss);
			return convert.from_bytes(oss.str());

			// Goddamn wstring seems to be the only way to preserve fancy chars...
		});

		emscripten::function("get_selection", +[]
		{
			auto a = emscripten::val::array();
			int i = 0;
			foreach (seq : editor_canvas->editor.getSelection())
				a.set(i++, tojsval(**seq, editor_canvas->editor.getGraph()));
			return a;
		});

		emscripten::function("get_pre_choices", +[]
		{
			Graph const & g = editor_canvas->editor.getGraph();
			auto const & selection = editor_canvas->editor.getSelection();
			vector<emscripten::val> items;
			if (!selection.empty())
				foreach (i : g[*from(selection.front(), g)].in)
					items.push_back(tojsval(*i, g));
			return tojsval(items);
		});

		emscripten::function("get_post_choices", +[]
		{
			Graph const & g = editor_canvas->editor.getGraph();
			auto const & selection = editor_canvas->editor.getSelection();
			vector<emscripten::val> items;
			if (!selection.empty())
				foreach (o : g[*to(selection.back(), g)].out)
					items.push_back(tojsval(*o, g));
			return tojsval(items);
		});

		emscripten::function("get_dirty", +[]
		{
			Graph const & g = editor_canvas->editor.getGraph();

			vector<emscripten::val>
				nodes_added, nodes_changed, edges_added, edges_changed;

			foreach(n : editor_canvas->dirty.nodes_added) nodes_added.push_back(tojsval(n, g));
			foreach(n : editor_canvas->dirty.nodes_changed) nodes_changed.push_back(tojsval(n, g));
			foreach(s : editor_canvas->dirty.edges_added) edges_added.push_back(tojsval(s, g));
			foreach(s : editor_canvas->dirty.edges_changed) edges_changed.push_back(tojsval(s, g));

			auto d = emscripten::val::object();
			d.set("nodes_added", tojsval(nodes_added));
			d.set("edges_added", tojsval(edges_added));
			d.set("nodes_changed", tojsval(nodes_changed));
			d.set("edges_changed", tojsval(edges_changed));
			return d;
		});

		emscripten::function("insert_keyframe", +[]
		{
			editor_canvas->editor.insert_keyframe();
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas);
		});

		emscripten::function("delete_keyframe", +[]
		{
			editor_canvas->editor.delete_keyframe();
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas);
		});

		emscripten::function("swap_players", +[]
		{
			swap_players(editor_canvas->editor);
			update_modified(*editor_canvas);
		});

		emscripten::function("prepend_new", +[](uint16_t const n)
		{
			editor_canvas->editor.prepend_new(NodeNum{n});
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas);
		});

		emscripten::function("append_new", +[](uint16_t const n)
		{
			editor_canvas->editor.append_new(NodeNum{n});
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas);
		});

		emscripten::function("browseto", +[](string const & desc)
		{
			if (auto start = named_entity(editor_canvas->editor.getGraph(), desc))
			{
				if (Step const * step = boost::get<Step>(&*start))
					go_to(**step, editor_canvas->editor);
				else if (NodeNum const * node = boost::get<NodeNum>(&*start))
					go_to(*node, editor_canvas->editor);
				else EM_ASM({ alert('todo'); });
			}
			else EM_ASM({ alert('Error: No such thing'); });

			ensure_nonempty_selection(editor_canvas->editor);
			update_selection_gui(*editor_canvas);
		});

		emscripten::function("goto_segment", +[](uint32_t const seq, uint16_t const seg)
		{
			go_to(SegmentInSequence{SeqNum{seq}, SegmentNum{uint8_t(seg)}}, editor_canvas->editor);
		});

		emscripten::function("goto_position", +[](uint32_t const seq, uint16_t const pos)
		{
			go_to(PositionInSequence{SeqNum{seq}, PosNum{uint8_t(pos)}}, editor_canvas->editor);
		});

		emscripten::function("set_node_desc", +[](string const & desc)
		{
			if (auto pis = position(editor_canvas->editor.getLocation()))
				if (auto n = node_at(editor_canvas->editor.getGraph(), **pis))
				{
					editor_canvas->editor.set_description(*n, desc);
					update_selection_gui(*editor_canvas);
					update_modified(*editor_canvas, true);
				}
		});

		emscripten::function("set_seq_desc", +[](string const & desc)
		{
			editor_canvas->editor.set_description(editor_canvas->editor.getLocation()->segment.sequence, desc);
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas, true);
		});

		emscripten::function("mirror_view", +[]
		{
			editor_canvas->editor.mirror();
		});

		emscripten::function("mirror_frame", +[]
		{
			mirror_position(editor_canvas->editor);
			update_modified(*editor_canvas);
		});

		emscripten::function("ignore_keyboard", +[](bool const b)
		{
			editor_canvas->ignore_keyboard = b;
		});

		emscripten::function("confine", +[](bool const b)
		{
			editor_canvas->editor.toggle_lock(b);
		});

		emscripten::function("mode", +[](string const & m)
		{
			set_playing(editor_canvas->editor, m == "playback");
			editor_canvas->edit_mode = (m == "edit");
		});

		emscripten::function("confine_interpolation", +[](bool const b)
		{
			editor_canvas->confine_interpolation = b;
		});

		emscripten::function("confine_horizontal", +[](bool const b)
		{
			editor_canvas->confine_horizontal = b;
		});

		emscripten::function("confine_local_edits", +[](bool const b)
		{
			editor_canvas->confine_local_edits = b;
		});

		emscripten::function("joints_to_edit", +[](string const & s)
		{
			editor_canvas->joints_to_edit = s;
		});

		emscripten::function("transform", +[](string const & s)
		{
			editor_canvas->transform_rotate = (s == "rotate");
		});

		emscripten::function("resolution", +[](int w, int h)
		{
			if (w > 128 && h > 128)
				glfwSetWindowSize(editor_canvas->window, w, h);
		});
		
		emscripten::function("undo", +[]
		{
			editor_canvas->editor.undo();
			update_selection_gui(*editor_canvas);
			update_modified(*editor_canvas);
		});

		emscripten::function("set_selected", +[](uint32_t const seq, bool const b)
		{
			SeqNum const sn{seq};

			Editor & editor = editor_canvas->editor;
			Graph const & graph = editor.getGraph();
			OrientedPath const & selection = editor.getSelection();

			editor_canvas->editor.set_selected(sn, b);

			if (auto pos = position(editor.getLocation()))
			if (auto node = node_at(graph, **pos))
				if (!selection.empty() && *node == *from(*selection.front(), graph))
					editor.setLocation(from_loc(first_segment(selection.front(), graph)));

			update_selection_gui(*editor_canvas);
		});

		emscripten::function("split_seq", +[]
		{
			editor_canvas->editor.branch();
			update_modified(*editor_canvas);
			update_selection_gui(*editor_canvas);
		});
	}
}
