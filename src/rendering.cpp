#include "rendering.hpp"
#include "util.hpp"
#include "graph.hpp"
#include "camera.hpp"
#include <GLFW/glfw3.h>

namespace
{
	void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
	void glVertex(V3 const & v) { ::glVertex3d(v.x, v.y, v.z); }
	void glTranslate(V3 const & v) { ::glTranslated(v.x, v.y, v.z); }
	void glColor(V3 v) { ::glColor3d(v.x, v.y, v.z); }

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

	void gluLookAt(V3 eye, V3 center, V3 up)
	{
		::gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z);
	}

	void grid()
	{
		glNormal3d(0, 1, 0);
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

	void render(Viables const * const viables, Position const & pos,
		boost::optional<PlayerJoint> const highlight_joint,
		boost::optional<unsigned> const first_person_player, bool const edit_mode)
	{
		auto limbs = [&](Player const & player)
			{
				foreach (s : segments())
					if (s.visible)
					{
						auto const a = s.ends[0], b = s.ends[1];
						pillar(player[a], player[b], jointDefs[a].radius, jointDefs[b].radius, 30);
					}
			};

		glColor(playerDefs[0].color);
		limbs(pos[0]);

		glColor(playerDefs[1].color);
		limbs(pos[1]);

		foreach (pj : playerJoints)
		{
			if (pj.player == first_person_player && pj.joint == Head) continue;

			auto color = playerDefs[pj.player].color;
			bool highlight = pj == highlight_joint;
			double extraBig = highlight ? 0.01 : 0.005;

			if (edit_mode)
				color = highlight ? yellow : white;
			else if (!viables || (*viables)[pj].total_dist == 0)
				extraBig = 0;
			else
				color = highlight
					? white * 0.7 + color * 0.3
					: white * 0.4 + color * 0.6;

			glColor(color);

			static GLUquadricObj * Sphere = gluNewQuadric(); // todo: evil
			glPushMatrix();
				glTranslate(pos[pj]);
				gluSphere(Sphere, jointDefs[pj.joint].radius + extraBig, 20, 20);
			glPopMatrix();
		}
	}

	void drawViables(Graph const & graph, Viables const & viable, PlayerJoint const j)
	{
		foreach (v : viable[j].viables)
		{
			if (v.second.end - v.second.begin < 1) continue;

			auto const r = v.second.reorientation;
			auto & seq = graph.sequence(v.first).positions;

			glColor4f(1, 1, 0, 0.3);

			glDisable(GL_DEPTH_TEST);

			glBegin(GL_LINE_STRIP);
			for (PosNum i = v.second.begin; i != v.second.end; ++i) glVertex(apply(r, seq[i][j]));
			glEnd();

			glPointSize(15);
			glBegin(GL_POINTS);
			for (PosNum i = v.second.begin; i != v.second.end; ++i) glVertex(apply(r, seq[i][j]));
			glEnd();
			glEnable(GL_DEPTH_TEST);
		}
	}
}

void renderWindow(
	Viables const * const viables,
	Graph const & graph,
	GLFWwindow * const window,
	Position const & position,
	Camera camera,
	boost::optional<PlayerJoint> highlight_joint,
	bool edit_mode)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	struct View
	{
		double x, y, w, h; // all in [0,1]
		boost::optional<unsigned> first_person;
	};

	View views[]
		{ View{0, 0, .5, 1, boost::none}
		, View{.5, .5, .5, .5, boost::optional<unsigned>(0)}
		, View{.5, 0, .5, .5, boost::optional<unsigned>(1)}
		};

	glEnable(GL_SCISSOR_TEST);

	foreach (v : views)
	{
		int
			x = v.x * width,
			y = v.y * height,
			w = v.w * width,
			h = v.h * height;

		glViewport(x, y, w, h);
		glScissor(x, y, w, h);

		camera.setViewportSize(w, h);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(camera.projection().data());

		glMatrixMode(GL_MODELVIEW);

		if (v.first_person)
		{
			auto const & p = position[*v.first_person];

			glLoadIdentity();
			gluLookAt(p[Head], (p[LeftHand] + p[RightHand]) / 2., p[Head] - p[Core]);
		}
		else
			glLoadMatrixd(camera.model_view().data());

		GLfloat light_diffuse[] = {0.5, 0.5, 0.5, 1.0};
		GLfloat light_ambient[] = {0.3, 0.3, 0.3, 0.0};
		GLfloat light_position[] = {1.0, 2.0, 1.0, 0.0};

		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);

		glEnable(GL_DEPTH);
		glEnable(GL_DEPTH_TEST);

		grid();
		render(viables, position, highlight_joint, v.first_person, edit_mode);

		if (viables && highlight_joint)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glLineWidth(4);
			glNormal3d(0, 1, 0);
			drawViables(graph, *viables, *highlight_joint);

			glDisable(GL_DEPTH_TEST);
			glPointSize(20);
			glColor(white);

			glBegin(GL_POINTS);
			glVertex(position[*highlight_joint]);
			glEnd();
		}
	};

	glfwSwapBuffers(window);
}
