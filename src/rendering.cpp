#include "rendering.hpp"
#include "util.hpp"
#include "graph.hpp"
#include "camera.hpp"
#ifdef USE_FTGL
#include <FTGL/ftgl.h>
#endif

namespace GrappleMap {

namespace
{
	void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
	void glVertex(V3 const & v) { ::glVertex3d(v.x, v.y, v.z); }
	void glTranslate(V3 const & v) { ::glTranslated(v.x, v.y, v.z); }
	void glColor(V3 v) { ::glColor3d(v.x, v.y, v.z); }

	void gluLookAt(V3 eye, V3 center, V3 up)
	{
		::gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z);
	}

	void drawViables(
		Graph const & graph, Viables const & viable, PlayerJoint const j,
		Selection const & selection,
		Camera const * camera, Style const & style)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(4);
		glNormal3d(0, 1, 0);

		foreach (v : viable[j].viables)
		{
			if (v.second.end.index - v.second.begin.index < 1) continue;

			auto const r = v.second.sequence.reorientation;
			auto & seq = graph[v.first];

			if (elem(*v.second.sequence, selection))
				glColor4f(1, 1, 1, 0.6);
			else
				glColor4f(1, 1, 0, 0.3);

			glDisable(GL_DEPTH_TEST);

			glBegin(GL_LINE_STRIP);
			for (PosNum i = v.second.begin; i != v.second.end; ++i) glVertex(apply(r, seq[i], j));
			glEnd();

			glPointSize(20);
			glBegin(GL_POINTS);
			for (PosNum i = v.second.begin; i != v.second.end; ++i)
				if (i.index == 0 || i.index == seq.positions.size() - 1)
					glVertex(apply(r, seq[i], j));
					// todo: this is silly
			glEnd();

			if (elem(*v.second.sequence, selection))
			{
				#ifdef USE_FTGL
				if (camera && !style.font.Error())
				{
					for (PosNum i = next(v.second.begin); i != v.second.end; ++i)
						renderText(
							style.font,
							world2screen(*camera, apply(r, seq[i], j)),
							to_string(i.index), white);
				}
				else
				#endif
				{
					glPointSize(10);
					glBegin(GL_POINTS);
					for (PosNum i = v.second.begin; i != v.second.end; ++i)
						if (i.index != 0 && i.index != seq.positions.size() - 1)
							glVertex(apply(r, seq[i], j));
					glEnd();
				}
			}

			glEnable(GL_DEPTH_TEST);
		}
	}
}

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

void renderWindow(
	std::vector<View> const & views,
	Viables const * const viables,
	Graph const & graph,
	Position const & position,
	Camera camera,
	optional<PlayerJoint> const highlight_joint,
	PerPlayerJoint<optional<V3>> colors,
	int const left, int const bottom,
	int const width, int const height,
	Selection const & selection,
	Style const & style,
	PlayerDrawer const & playerDrawer)
{
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_POINT_SMOOTH);

	foreach (v : views)
	{
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

		if (viables && highlight_joint)
		{
			drawViables(graph, *viables, *highlight_joint, selection, &camera, style);

			glDisable(GL_DEPTH_TEST);
			glPointSize(20);
			glColor(white);

			glBegin(GL_POINTS);
			glVertex(position[*highlight_joint]);
			glEnd();
		}
	}
}

void renderScene(
	Graph const & graph,
	Position const & position,
	PerPlayerJoint<ViablesForJoint> const & viables,
	optional<PlayerJoint> const browse_joint,
	optional<PlayerJoint> const edit_joint,
	Selection const & selection,
	Style const & style,
	PlayerDrawer const & playerDrawer)
{
	glEnable(GL_COLOR_MATERIAL);

	setupLights();

	grid(style.grid_color, style.grid_size, style.grid_line_width);

	{
		PerPlayerJoint<optional<V3>> colors;

		foreach (j : playerJoints)
			if (viables[j].total_dist != 0)
				colors[j] = white * 0.4 + playerDefs[j.player].color * 0.6;

		if (browse_joint) colors[*browse_joint] = V3{1, 1, 0};
		if (edit_joint) colors[*edit_joint] = V3{0, 1, 0};

		playerDrawer.drawPlayers(position, colors, {});
	}

	if (edit_joint)
	{
		auto const j = *edit_joint;

		drawViables(graph, viables, j, selection, nullptr, style);

		glDisable(GL_DEPTH_TEST);
		glPointSize(20);
		glColor(white);

		glBegin(GL_POINTS);
		glVertex(position[j]);
		glEnd();
	}

	if (browse_joint)
	{
		auto const j = *browse_joint;

		drawViables(graph, viables, j, selection, nullptr, style);

		glDisable(GL_DEPTH_TEST);
		glPointSize(20);
		glColor(white);

		glBegin(GL_POINTS);
		glVertex(position[j]);
		glEnd();
	}
}

}
