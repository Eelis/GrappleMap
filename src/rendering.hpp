#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

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

void render(Viables const & viables, Position const & pos,
	PlayerJoint const highlight_joint, bool const edit_mode)
{
	auto limbs = [&](Player const & player)
		{
			foreach (s : segments)
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
		auto color = playerDefs[pj.player].color;
		bool highlight = pj == highlight_joint;
		double extraBig = highlight ? 0.01 : 0.005;

		if (edit_mode)
			color = highlight ? yellow : white;
		else if (viables[pj].total_dist == 0)
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

		glColor4f(1, 1, 0, 0.5);

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

#endif
