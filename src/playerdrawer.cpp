#include "playerdrawer.hpp"
#include <GL/glu.h>

namespace GrappleMap {

namespace
{
	void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
	void glVertex(V3 const & v) { ::glVertex3d(v.x, v.y, v.z); }
	void glColor(V3 v) { ::glColor3d(v.x, v.y, v.z); }

	void fatTriangle(V3 a, double arad, V3 b, double brad, V3 c, double crad)
	{
		auto const
			ab = normalize(b - a),
			bc = normalize(c - b),
			ca = normalize(a - c),
			fwd = normalize(cross(b - a, c - a));

		auto const
			aleft = a + ca * arad,
			adown = a - ab * arad,
			cright = c - ca * crad,
			cdown = c + bc * crad,
			bleft = b - bc * brad,
			bright = b + ab * brad,
			afwd = a + fwd * arad,
			aback = a - fwd * arad,
			bfwd = b + fwd * brad,
			bback = b - fwd * brad,
			cfwd = c + fwd * crad,
			cback = c - fwd * crad;

		
		glBegin(GL_TRIANGLES);
			// front
			glNormal(fwd);
			glVertex(afwd);
			glNormal(fwd);
			glVertex(bfwd);
			glNormal(fwd);
			glVertex(cfwd);

			// front left
			glNormal(fwd);
			glVertex(afwd);
			glNormal(ca);
			glVertex(aleft);
			glNormal(-bc);
			glVertex(bleft);

			glNormal(fwd);
			glVertex(afwd);
			glNormal(-bc);
			glVertex(bleft);
			glNormal(fwd);
			glVertex(bfwd);

			// front right
			glNormal(fwd);
			glVertex(bfwd);
			glNormal(ab);
			glVertex(bright);
			glNormal(-ca);
			glVertex(cright);

			glNormal(fwd);
			glVertex(bfwd);
			glNormal(-ca);
			glVertex(cright);
			glNormal(fwd);
			glVertex(cfwd);

			// front bottom
			glNormal(fwd);
			glVertex(cfwd);
			glNormal(bc);
			glVertex(cdown);
			glNormal(-ab);
			glVertex(adown);

			glNormal(fwd);
			glVertex(cfwd);
			glNormal(-ab);
			glVertex(adown);
			glNormal(fwd);
			glVertex(afwd);

			// back
			glNormal(-fwd);
			glVertex(cback);
			glNormal(-fwd);
			glVertex(bback);
			glNormal(-fwd);
			glVertex(aback);

			// back left
			glNormal(-fwd);
			glVertex(aback);
			glNormal(-fwd);
			glVertex(bback);
			glNormal(-bc);
			glVertex(bleft);

			glNormal(-fwd);
			glVertex(aback);
			glNormal(-bc);
			glVertex(bleft);
			glNormal(ca);
			glVertex(aleft);

			// back right
			glNormal(-fwd);
			glVertex(bback);
			glNormal(-fwd);
			glVertex(cback);
			glNormal(-ca);
			glVertex(cright);

			glNormal(-fwd);
			glVertex(bback);
			glNormal(-ca);
			glVertex(cright);
			glNormal(ab);
			glVertex(bright);

			// back bottom
			glNormal(-fwd);
			glVertex(cback);
			glNormal(-fwd);
			glVertex(aback);
			glNormal(-ab);
			glVertex(adown);

			glNormal(-fwd);
			glVertex(cback);
			glNormal(-ab);
			glVertex(adown);
			glNormal(bc);
			glVertex(cdown);
		glEnd();

		// todo: (indexed?) triangle strip
	}

	void fatTriangle(Position const & p, Joint const x, Joint y, Joint z)
	{
		foreach (n : playerNums())
		{
			glColor(playerDefs[n].color);
			fatTriangle(
				p[PlayerJoint{n, x}], jointDefs[x].radius,
				p[PlayerJoint{n, y}], jointDefs[y].radius,
				p[PlayerJoint{n, z}], jointDefs[z].radius);
		}
	}
}

PlayerDrawer::PlayerDrawer()
	: fine_icomesh{icosphere::make_icosphere(2)}
	, course_icomesh{icosphere::make_icosphere(1)}
{
	double const s = 2 * pi() / faces;

	for (unsigned i = 0; i != faces + 1; ++i)
		angles[i] = std::make_pair(sin(i * s), cos(i * s));
}

void PlayerDrawer::drawSphere(V3 center, double radius, bool const fine) const
{
	auto const & mesh = fine ? fine_icomesh : course_icomesh;

	auto f = [&](unsigned i)
		{
			auto v = mesh.first[i];
			glNormal(v);
			glVertex(center + v * radius);
		};

	glBegin(GL_TRIANGLES);
		foreach (t : mesh.second)
		{
			
			f(t.vertex[2]);
			f(t.vertex[1]);
			f(t.vertex[0]);
		}
	glEnd();
}

void PlayerDrawer::drawPillar(V3 from, V3 to, double from_radius, double to_radius) const
{
	V3 a = normalize(cross(to - from, V3{1,1,1} - from));
	V3 b = normalize(cross(to - from, a));

	glBegin(GL_TRIANGLE_STRIP);

	for (unsigned i = 0; i != faces; ++i)
	{
		glNormal(a * angles[i].first + b * angles[i].second);
		glVertex(from + a * from_radius * angles[i].first + b * from_radius * angles[i].second);
		glVertex(to   + a *   to_radius * angles[i].first + b *   to_radius * angles[i].second);

		glNormal(a * angles[i+1].first + b * angles[i+1].second);
		glVertex(from + a * from_radius * angles[i+1].first + b * from_radius * angles[i+1].second);

		glVertex(from + a * from_radius * angles[i+1].first + b * from_radius * angles[i+1].second);
		glVertex(to   + a *   to_radius * angles[i+1].first + b *   to_radius * angles[i+1].second);

		glNormal(a * angles[i].first + b * angles[i].second);
		glVertex(to   + a *   to_radius * angles[i].first + b *   to_radius * angles[i].second);
	}

	glEnd();
}

void PlayerDrawer::drawJoints(Position const & pos,
	PerPlayerJoint<optional<V3>> const & colors,
	optional<PlayerNum> const first_person_player) const
{
	foreach (pj : playerJoints)
	{
		if (pj.player == first_person_player && pj.joint == Head) continue;

		auto color = colors[pj];

		double extraBig = 0;

		if (color)
		{
			extraBig = 0.005;
			glColor(*color);
		}
		else glColor(playerDefs[pj.player].color);

		drawSphere(pos[pj], jointDefs[pj.joint].radius + extraBig,
			pj.joint == Head || pj.joint == Core ||
			pj.joint == RightHip || pj.joint == LeftHip ||
			pj.joint == RightShoulder || pj.joint == LeftShoulder);
	}
}

void PlayerDrawer::drawLimbs(Position const & pos, optional<PlayerNum> const first_person_player) const
{
	foreach (p : playerNums())
	{
		glColor(playerDefs[p].color);
		Player const & player = pos[p];

		foreach (l : limbs())
			if (l.visible)
			{
				auto const a = l.ends[0], b = l.ends[1];

				if (b == Head && p == first_person_player) continue;

				if (l.midpointRadius)
				{
					auto mid = (player[a] + player[b]) / 2;
					drawPillar(player[a], mid, jointDefs[a].radius, *l.midpointRadius);
					drawPillar(mid, player[b], *l.midpointRadius, jointDefs[b].radius);
						// todo: can be done faster than one pillar at a time
				}
				else
					drawPillar(player[a], player[b], jointDefs[a].radius, jointDefs[b].radius);
			}
	}
}

void PlayerDrawer::drawPlayers(Position const & pos,
	PerPlayerJoint<optional<V3>> const & colors,
	optional<PlayerNum> const first_person_player) const
{
	drawLimbs(pos, first_person_player);
	drawJoints(pos, colors, first_person_player);

	fatTriangle(pos, LeftHip, Core, RightHip);
	fatTriangle(pos, LeftShoulder, Neck, RightShoulder);
	fatTriangle(pos, LeftShoulder, Core, RightShoulder);
	fatTriangle(pos, LeftAnkle, LeftHeel, LeftToe);
	fatTriangle(pos, RightAnkle, RightHeel, RightToe);
}

}
