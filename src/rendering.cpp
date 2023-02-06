#include "rendering.hpp"
#include "graph_util.hpp"
#include "camera.hpp"

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifdef USE_FTGL
#include <FTGL/ftgl.h>
#endif

namespace GrappleMap {

namespace
{
	#ifndef EMSCRIPTEN
	void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
	void glVertex(V3 const & v) { ::glVertex3d(v.x, v.y, v.z); }
	void glTranslate(V3 const & v) { ::glTranslated(v.x, v.y, v.z); }
	void glColor(V3 v) { ::glColor3d(v.x, v.y, v.z); }
	#endif

	void gluLookAt(V3 eye, V3 center, V3 up)
	{
		::gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z);
	}

	void drawLine(V4f color, V4f color2, V3 from, V3 to, std::vector<BasicVertex> & out)
	{
		float const radius = 0.0025;

		V3f fromf = to_f(from);
		V3f tof = to_f(to);

		V3f a = normalize(cross(tof - fromf, V3f{1,1,1} - fromf));
		V3f b = normalize(cross(tof - fromf, a));

		out.push_back({ fromf + a * radius, b, color });
		out.push_back({ tof   + a * radius, b, color2 }),
		out.push_back({ tof   - a * radius, b, color2 });

		out.push_back({ tof   - a * radius, b, color2 });
		out.push_back({ fromf - a * radius, b, color });
		out.push_back({ fromf + a * radius, b, color });

		out.push_back({ fromf + b * radius, a, color });
		out.push_back({ tof   + b * radius, a, color2 }),
		out.push_back({ tof   - b * radius, a, color2 });

		out.push_back({ tof   - b * radius, a, color2 });
		out.push_back({ fromf - b * radius, a, color });
		out.push_back({ fromf + b * radius, a, color });
	}

	#ifndef EMSCRIPTEN
	void drawSelection(
		Graph const & g, OrientedPath const & path, PlayerJoint const j,
		optional<SegmentInSequence> const current_segment,
		Camera const * const camera, Style const & style)
	{
		glNormal3d(0, 1, 0);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(4);

		glColor4f(1, 1, 1, 0.6);

		foreach (seq : path)
		{
			glBegin(GL_LINE_STRIP);
				foreach (p : positions(forget_direction(seq), g))
				{
				
					if (current_segment && to(*current_segment) == *p)
						glBegin(GL_LINE_STRIP);

					glVertex(at(p, j, g));

					if (current_segment && from(*current_segment) == *p)
						glEnd();
				}
			glEnd();
		}
		#ifdef USE_FTGL
		if (camera && !style.font.Error())
			foreach (seq : path)
				foreach (p : positions(forget_direction(seq), g))
					renderText(
						style.font,
						world2screen(*camera, at(p, j, g)),
						to_string(p->position.index + 1), white);
		else
		#endif
		{
			glPointSize(10);
			glBegin(GL_POINTS);
			foreach (seq : path)
				foreach (v : joint_positions(forget_direction(seq), j, g))
//				if (p.index != 0 && i.index != seq.positions.size() - 1)
					glVertex(v);
			glEnd();
		}
	}
	#endif

	void drawSelection(
		SphereDrawer const & sphereDrawer,
		Graph const & g, OrientedPath const & path, PlayerJoint const j,
		SegmentInSequence const current_segment,
		Style const &,
		vector<BasicVertex> & out)
	{
		foreach (sn : path)
		{
			foreach (seg : segments(forget_direction(sn), g))
			{
				auto color = (current_segment == *seg ? V3f{0,1,0} : V3f{1,1,1});
				drawLine(
					V4f(color,0.6), V4f(color,0.6),
					at(from(seg), j, g), at(to(seg), j, g),
					out);
			}
		}

		foreach (seq : path)
		{
			for (Reoriented<PositionInSequence> p : positions(forget_direction(seq), g))
			{
				auto const v = at(p, j, g);

				bool const on_current_segment =
					(*p == from(current_segment) || *p == to(current_segment));

				auto color = on_current_segment ? V3f{0,1,0} : V3f{1,1,1};

				drawSphere(sphereDrawer, V4f(color, 0.5), v, 0.01, false, out);
			}
		}
	}

	#ifndef EMSCRIPTEN
	void drawViables(
		Graph const & graph, vector<Viable> const & viables,
		OrientedPath const & selection,
		optional<SegmentInSequence> const current_segment,
		Style const &)
	{
		glNormal3d(0, 1, 0);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(4);

		foreach (v : viables)
		{
			if (elem(*v.sequence, selection))
			{
				if (current_segment && current_segment->sequence == *v.sequence)
				{
					glColor4f(0, 1, 0, 0.8);
					glBegin(GL_LINES);
						glVertex(at(v.sequence * from(current_segment->segment), v.joint, graph));
						glVertex(at(v.sequence * to(current_segment->segment), v.joint, graph));
					glEnd();
				}
			}
			else
			{
				glBegin(GL_LINE_STRIP);

				foreach (i : PosNum::range(v.begin, v.end))
				{
					glColor4f(1, 1, 0, std::max(0.0, 0.3 - v.depth(i) * 0.05));
					glVertex(at(v.sequence * i, v.joint, graph));
				}
				glEnd();
			}
/*
			if (selected) glColor4f(1, 1, 1, 0.6);
			else glColor4f(1, 1, 0, 0.3);

			glPointSize(20);
			glBegin(GL_POINTS);
			if (v.begin.index == 0)
				glVertex(apply(r, seq.positions.front(), j));
			if (v.end.index == seq.positions.size())
				glVertex(apply(r, seq.positions.back(), j));
			glEnd();
*/
		}

		glEnable(GL_DEPTH_TEST);
	}
	#endif

	void drawViables(
		Graph const & graph, vector<Viable> const & viables,
		OrientedPath const & selection,
		Style const &,
		vector<BasicVertex> & out)
	{
		foreach (v : viables)
		{
			if (!elem(*v.sequence, selection))
			{
				foreach (seg : SegmentNum::range(SegmentNum{uint_fast8_t(v.begin.index)}, SegmentNum{uint_fast8_t(v.end.index - 1)}))
					// todo: nasty
				{
					auto rs = v.sequence * seg;

					drawLine(
						V4f(V3f{1, 1, 0}, std::max(0.0, 0.3 - v.depth(from(seg)) * 0.05)),
						V4f(V3f{1, 1, 0}, std::max(0.0, 0.3 - v.depth(to(seg)) * 0.05)),
						at(from(rs), v.joint, graph),
						at(to(rs), v.joint, graph),
						out);
				}

/* todo:
				foreach (i : PosNum::range(v.begin, v.end))
				{
					glColor4f(1, 1, 0, std::max(0.0, 0.3 - v.depth(i) * 0.05));
					glVertex(at(v.sequence * i, v.joint, graph));
				}
 */
			}
		}
	}
}

#ifndef EMSCRIPTEN
void setupLights()
{
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);

	GLfloat light_diffuse[] = {0.65, 0.65, 0.65, 1.0};
	GLfloat light_ambient[] = {0.03, 0.03, 0.03, 0.0};
	GLfloat light_position0[] = {2.0, 2.0, 2.0, 0.0};
	GLfloat light_position1[] = {-2.0, 2.0, -2.0, 0.0};

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);

	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
}

void grid(V3 const color, unsigned const size, unsigned const line_width)
{
	glNormal3d(0, 1, 0);
	glColor3f(color.x, color.y, color.z);
	glLineWidth(line_width);

	glBegin(GL_LINES);
		for (double i = size*-2.; i <= size*2.; ++i)
		{
			glVertex(V3{i/2, 0, size*-1.});
			glVertex(V3{i/2, 0, size*1.});
			glVertex(V3{size*-1., 0, i/2});
			glVertex(V3{size*1., 0, i/2});
		}
	glEnd();
}
#endif

void grid(V3f const col, unsigned const size, vector<BasicVertex> & out)
{
	V4f color(col, 1);

	for (float i = size*-2.; i <= size*2.; ++i)
	{
		out.push_back({ {i/2.f+.01f,0,size*-1.f}, {0,1,0}, color });
		out.push_back({ {i/2.f-.01f,0,size*-1.f}, {0,1,0}, color });
		out.push_back({ {i/2.f+.01f,0,size* 1.f}, {0,1,0}, color });

		out.push_back({ {i/2.f-.01f,0,size*-1.f}, {0,1,0}, color });
		out.push_back({ {i/2.f-.01f,0,size* 1.f}, {0,1,0}, color });
		out.push_back({ {i/2.f+.01f,0,size* 1.f}, {0,1,0}, color });

		out.push_back({ {size*-1.f,0,i/2.f+.01f}, {0,1,0}, color });
		out.push_back({ {size* 1.f,0,i/2.f+.01f}, {0,1,0}, color });
		out.push_back({ {size*-1.f,0,i/2.f-.01f}, {0,1,0}, color });

		out.push_back({ {size*-1.f,0,i/2.f-.01f}, {0,1,0}, color });
		out.push_back({ {size* 1.f,0,i/2.f+.01f}, {0,1,0}, color });
		out.push_back({ {size* 1.f,0,i/2.f-.01f}, {0,1,0}, color });
	}
}

Style::Style()
{
	#ifdef USE_FTGL
		if (!font.Error())
			font.FaceSize(16);
	#endif
}

#ifdef USE_FTGL
	void renderText(FTGLPixmapFont const & font, V2 where, string const & text, V3 const color)
	{
		glPixelTransferf(GL_RED_BIAS, color.x - 1);
		glPixelTransferf(GL_GREEN_BIAS, color.y - 1);
		glPixelTransferf(GL_BLUE_BIAS, color.z - 1);

		const_cast<FTGLPixmapFont &>(font)
			.Render(text.c_str(), -1, FTPoint(where.x, where.y, 0));
	}
#endif

void renderBasic(
	View const & v,
	Position const & position,
	Camera camera,
	PerPlayerJoint<optional<V3>> colors,
	int const left, int const bottom,
	int const width, int const height,
	Style const & style,
	PlayerDrawer const & playerDrawer)
{
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_POINT_SMOOTH);

	int
		x = left + v.x * width,
		y = bottom + v.y * height,
		w = v.w * width,
		h = v.h * height;

	glViewport(x, y, w, h);
	glScissor(x, y, w, h);

	camera.setViewportSize(v.fov, w, h);

	glClearColor(style.background_color.x, style.background_color.y, style.background_color.z, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_COLOR_MATERIAL);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(camera.projection().data());

	glMatrixMode(GL_MODELVIEW);

	if (v.first_person)
	{
		auto const & p = position[*v.first_person];

		glLoadIdentity();
		gluLookAt(p[Head], (p[LeftHand] + p[RightHand]) / 2., p[Head] - p[Neck]);
	}
	else
		glLoadMatrixd(camera.model_view().data());

	setupLights();

	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);

	grid(style.grid_color, style.grid_size, style.grid_line_width);

	playerDrawer.drawPlayers(position, colors, v.first_person);
}

#ifndef EMSCRIPTEN
void renderWindow(
	std::vector<View> const & views,
	vector<Viable> const & viables,
	Graph const & graph,
	Position const & position,
	Camera camera,
	optional<PlayerJoint> const highlight_joint,
	PerPlayerJoint<optional<V3>> colors,
	int const left, int const bottom,
	int const width, int const height,
	OrientedPath const & selection,
	Style const & style,
	PlayerDrawer const & playerDrawer,
	function<void()> extraRender)
{
	foreach (v : views)
	{
		renderBasic(v, position, camera, colors,
			left, bottom, width, height, style, playerDrawer);

		if (extraRender) extraRender();

		if (highlight_joint)
			drawSelection(
				graph, selection, *highlight_joint,
				boost::none, &camera, style);

		drawViables(graph, viables, selection, {}, style);

		if (highlight_joint)
		{
			glDisable(GL_DEPTH_TEST);
			glPointSize(20);
			glColor(white);

			glBegin(GL_POINTS);
			glVertex(position[*highlight_joint]);
			glEnd();
		}
	}
}
#endif

size_t renderWindow(
	std::vector<View> const & views,
	vector<Viable> const & viables,
	Graph const & graph,
	Position const & position,
	Camera camera,
	optional<PlayerJoint> const highlight_joint,
	PerPlayerJoint<optional<V3>> colors,
	int /*left*/, int /*bottom*/,
	int const width, int const height,
	OrientedPath const & selection,
	SegmentInSequence const current_segment,
	Style const & style,
	PlayerDrawer const & playerDrawer,
	function<void()> extraRender,
	vector<BasicVertex> & out)
{
	size_t num = 0;

	foreach (v : views)
	{
		int
			//x = left + v.x * width,
			//y = bottom + v.y * height,
			w = v.w * width,
			h = v.h * height;

		camera.setViewportSize(v.fov, w, h);

		grid(to_f(style.grid_color), style.grid_size, out);

		playerDrawer.drawPlayers(position, colors, v.first_person, out);

		if (extraRender) extraRender();

		num = out.size();

		if (highlight_joint)
			drawSelection(
				playerDrawer.sphereDrawer,
				graph, selection, *highlight_joint,
				current_segment, style, out);

		drawViables(graph, viables, selection, style, out);
	}

	return num;
}

#ifndef EMSCRIPTEN
void renderScene(
	Graph const & graph,
	Position const & position,
	vector<Viable> const & viables,
	optional<PlayerJoint> const browse_joint,
	vector<PlayerJoint> const & edit_joints,
	OrientedPath const & selection,
	PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> const & accessibleSegments,
	optional<SegmentInSequence> const current_segment,
	Style const & style,
	PlayerDrawer const & playerDrawer)
{
	glEnable(GL_COLOR_MATERIAL);

	setupLights();

	grid(style.grid_color, style.grid_size, style.grid_line_width);

	{
		PerPlayerJoint<optional<V3>> colors;

		if (browse_joint) colors[*browse_joint] = V3{1, 1, 0};

		foreach (j : playerJoints)
		{
			if (elem(j, edit_joints)) colors[j] = V3{0, 1, 0};

			if (!colors[j] && !accessibleSegments[j].empty())
				colors[j] = V3(white) * 0.4 + playerDefs[j.player].color * 0.6;
		}

		playerDrawer.drawPlayers(position, colors, {});
	}

	if (edit_joints.size() == 1)
		drawSelection(graph, selection, edit_joints.front(), current_segment, nullptr, style);
	if (browse_joint)
		drawSelection(graph, selection, *browse_joint, current_segment, nullptr, style);

	drawViables(graph, viables, selection, current_segment, style);

	glDisable(GL_DEPTH_TEST);
	glPointSize(20);
	glColor(white);

	glBegin(GL_POINTS);
	//foreach (j : edit_joints) glVertex(position[j]);
	if (browse_joint) glVertex(position[*browse_joint]);
	glEnd();
}
#endif

}
