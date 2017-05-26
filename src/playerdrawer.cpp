#include "playerdrawer.hpp"
#include <GL/glu.h>

namespace GrappleMap {

namespace
{
	#ifndef EMSCRIPTEN
	void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
	void glVertex(V3 const & v) { ::glVertex3d(v.x, v.y, v.z); }
	void glColor(V3 v) { ::glColor3d(v.x, v.y, v.z); }
	#endif

	template<typename F>
	void fatTriangle(V3 a, double arad, V3 b, double brad, V3 c, double crad, F out)
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
		
		// front
		out(afwd, fwd);
		out(bfwd, fwd);
		out(cfwd, fwd);

		// front left
		out(afwd, fwd);
		out(aleft, ca);
		out(bleft, -bc);

		out(afwd, fwd);
		out(bleft, -bc);
		out(bfwd, fwd);

		// front right
		out(bfwd, fwd);
		out(bright, ab);
		out(cright, -ca);

		out(bfwd, fwd);
		out(cright, -ca);
		out(cfwd, fwd);

		// front bottom
		out(cfwd, fwd);
		out(cdown, bc);
		out(adown, -ab);

		out(cfwd, fwd);
		out(adown, -ab);
		out(afwd, fwd);

		// back
		out(cback, -fwd);
		out(bback, -fwd);
		out(aback, -fwd);

		// back left
		out(aback, -fwd);
		out(bback, -fwd);
		out(bleft, -bc);

		out(aback, -fwd);
		out(bleft, -bc);
		out(aleft, ca);

		// back right
		out(bback, -fwd);
		out(cback, -fwd);
		out(cright, -ca);

		out(bback, -fwd);
		out(cright, -ca);
		out(bright, ab);

		// back bottom
		out(cback, -fwd);
		out(aback, -fwd);
		out(adown, -ab);

		out(cback, -fwd);
		out(adown, -ab);
		out(cdown, bc);
	}

	#ifndef EMSCRIPTEN
	void fatTriangle(V3 a, double arad, V3 b, double brad, V3 c, double crad)
	{
		glBegin(GL_TRIANGLES);
			fatTriangle(a, arad, b, brad, c, crad,
				[&](V3 pos, V3 norm) { glNormal(norm); glVertex(pos); });
		glEnd();
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
	#endif

	void fatTriangle(V3 color, V3 a, double arad, V3 b, double brad, V3 c, double crad, std::vector<BasicVertex> & out)
	{
		fatTriangle(a, arad, b, brad, c, crad, [&](V3f pos, V3f norm)
			{
				out.push_back({pos, norm, V4f(color, 1)});
			});
	}

	void fatTriangle(Position const & p, Joint const x, Joint y, Joint z, std::vector<BasicVertex> & out)
	{
		foreach (n : playerNums())
		{
			fatTriangle(playerDefs[n].color,
				p[PlayerJoint{n, x}], jointDefs[x].radius,
				p[PlayerJoint{n, y}], jointDefs[y].radius,
				p[PlayerJoint{n, z}], jointDefs[z].radius, out);
		}
	}
}

SphereDrawer::SphereDrawer()
	: fine_icomesh{icosphere::make_icosphere(2)}
	, course_icomesh{icosphere::make_icosphere(1)}
{}

template<typename F>
void SphereDrawer::draw(V3 center, double radius, bool const fine, F out) const
{
	auto const & mesh = fine ? fine_icomesh : course_icomesh;

	auto f = [&](unsigned i)
		{
			auto v = mesh.first[i];
			out(center + v * radius, v);
		};

	foreach (t : mesh.second)
	{
		f(t.vertex[2]);
		f(t.vertex[1]);
		f(t.vertex[0]);
	}
}

#ifndef EMSCRIPTEN
void drawSphere(SphereDrawer const & d, V3 center, double radius, bool const fine)
{
	glBegin(GL_TRIANGLES);
		d.draw(center, radius, fine,
			[&](V3 pos, V3 norm) { glNormal(norm); glVertex(pos); });
	glEnd();
}
#endif

PlayerDrawer::PlayerDrawer()
{
	double const s = 2 * pi() / faces;

	for (unsigned i = 0; i != faces + 1; ++i)
		angles[i] = std::make_pair(sin(i * s), cos(i * s));
}

void drawSphere(SphereDrawer const & sd, V4f color, V3 center, double radius, bool const fine, std::vector<BasicVertex> & out)
{
	sd.draw(center, radius, fine, [color, &out](V3f pos, V3f norm)
		{
			out.push_back({pos, norm, color});
		});
}

#ifndef EMSCRIPTEN
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
#endif

template<typename F>
void PlayerDrawer::drawPillar(V3 from, V3 to, double from_radius, double to_radius, F out) const
{
	V3 a = normalize(cross(to - from, V3{1,1,1} - from));
	V3 b = normalize(cross(to - from, a));

	for (unsigned i = 0; i != faces; ++i)
	{
		V3 normal = a * angles[i].first + b * angles[i].second;
		V3 normal2 = a * angles[i+1].first + b * angles[i+1].second;

		out(from + a * from_radius * angles[i  ].first + b * from_radius * angles[i  ].second, normal);
		out(to   + a *   to_radius * angles[i  ].first + b *   to_radius * angles[i  ].second, normal);
		out(from + a * from_radius * angles[i+1].first + b * from_radius * angles[i+1].second, normal2);

		out(from + a * from_radius * angles[i+1].first + b * from_radius * angles[i+1].second, normal2);
		out(to   + a *   to_radius * angles[i  ].first + b *   to_radius * angles[i  ].second, normal);
		out(to   + a *   to_radius * angles[i+1].first + b *   to_radius * angles[i+1].second, normal2);
	}
}

void PlayerDrawer::drawPillar(V3 color, V3 from, V3 to, double from_radius, double to_radius, std::vector<BasicVertex> & out) const
{
	drawPillar(from, to, from_radius, to_radius,
		[&out, color](V3f pos, V3f norm){ out.push_back({pos, norm, V4f(color, 1)}); });
}

#ifndef EMSCRIPTEN
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

		drawSphere(sphereDrawer, pos[pj], jointDefs[pj.joint].radius + extraBig,
			pj.joint == Head || pj.joint == Core ||
			pj.joint == RightHip || pj.joint == LeftHip ||
			pj.joint == RightShoulder || pj.joint == LeftShoulder);
	}
}
#endif

void PlayerDrawer::drawJoints(Position const & pos,
	PerPlayerJoint<optional<V3>> const & colors,
	optional<PlayerNum> const first_person_player,
	std::vector<BasicVertex> & out) const
{
	foreach (pj : playerJoints)
	{
		if (pj.player == first_person_player && pj.joint == Head) continue;

		auto color = colors[pj];

		double extraBig = 0;

		V3 cc = playerDefs[pj.player].color;

		if (color)
		{
			extraBig = 0.005;
			cc = *color;
		}

		drawSphere(sphereDrawer, V4f(to_f(cc), 1), pos[pj], jointDefs[pj.joint].radius + extraBig,
			pj.joint == Head || pj.joint == Core ||
			pj.joint == RightHip || pj.joint == LeftHip ||
			pj.joint == RightShoulder || pj.joint == LeftShoulder, out);
	}
}

#ifndef EMSCRIPTEN
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
					auto mid = (player[a] + player[b]) / 2.;
					drawPillar(player[a], mid, jointDefs[a].radius, *l.midpointRadius);
					drawPillar(mid, player[b], *l.midpointRadius, jointDefs[b].radius);
						// todo: can be done faster than one pillar at a time
				}
				else
					drawPillar(player[a], player[b], jointDefs[a].radius, jointDefs[b].radius);
			}
	}
}
#endif

void PlayerDrawer::drawLimbs(Position const & pos, optional<PlayerNum> const first_person_player,
	vector<BasicVertex> & out) const
{
	foreach (p : playerNums())
	{
		V3 const color = playerDefs[p].color;
		Player const & player = pos[p];

		foreach (l : limbs())
			if (l.visible)
			{
				auto const a = l.ends[0], b = l.ends[1];

				if (b == Head && p == first_person_player) continue;

				if (l.midpointRadius)
				{
					auto mid = (player[a] + player[b]) / 2.;
					drawPillar(color, player[a], mid, jointDefs[a].radius, *l.midpointRadius, out);
					drawPillar(color, mid, player[b], *l.midpointRadius, jointDefs[b].radius, out);
						// todo: can be done faster than one pillar at a time
				}
				else
					drawPillar(color, player[a], player[b], jointDefs[a].radius, jointDefs[b].radius, out);
			}
	}
}

#ifndef EMSCRIPTEN
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
#endif

void PlayerDrawer::drawPlayers(Position const & pos,
	PerPlayerJoint<optional<V3>> const & colors,
	optional<PlayerNum> const first_person_player, vector<BasicVertex> & out) const
{
	drawLimbs(pos, first_person_player, out);
	
	drawJoints(pos, colors, first_person_player, out);
	fatTriangle(pos, LeftHip, Core, RightHip, out);
	fatTriangle(pos, LeftShoulder, Neck, RightShoulder, out);
	fatTriangle(pos, LeftShoulder, Core, RightShoulder, out);
	fatTriangle(pos, LeftAnkle, LeftHeel, LeftToe, out);
	fatTriangle(pos, RightAnkle, RightHeel, RightToe, out);
}

}
