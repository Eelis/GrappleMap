#ifndef GRAPPLEMAP_POSITIONS_HPP
#define GRAPPLEMAP_POSITIONS_HPP

#include "math.hpp"
#include "players.hpp"

namespace GrappleMap {

using Position = PerPlayerJoint<V3>;

struct Sequence
{
	vector<string> description;
	vector<Position> positions;
		// invariant: .size()>=2
	optional<unsigned> line_nr;
	bool detailed, bidirectional;

	Position const & operator[](PosNum const n) const { return positions[n.index]; }
};

inline Position operator+(Position r, V3 const off)
{
	for (auto j : playerJoints) r[j] += off;
	return r;
}

inline Position operator-(Position const & p, V3 const off)
{
	return p + -off;
}

bool top_is_on_bottoms_left_side(Position const &);

inline V3 mirror(V3 v) { return V3{-v.x, v.y, v.z}; }
Position mirror(Position);

Player spring(Player const &, optional<Joint> fixed_joint = none);
void spring(Position &, optional<PlayerJoint> = none);

inline Position between(Position const & a, Position const & b, double s = 0.5 /* [0,1] */)
{
	Position r;
	foreach (j : playerJoints) r[j] = a[j] + (b[j] - a[j]) * s;
	return r;
}

inline bool basicallySame(Position const & a, Position const & b)
{
	double u = 0;
	foreach (j : playerJoints) u += distanceSquared(a[j], b[j]);
	return u < 0.03;
}

template<typename... A>
bool basicallySame(Position const & a, Position const & b, A const & ... more)
{
	return basicallySame(a, b) && basicallySame(b, more...);
}

inline Position apply(Reorientation const & r, Position p)
{
	foreach (j : playerJoints) p[j] = apply(r, p[j]);
	return p;
}

inline void swap_players(Position & p)
{
	std::swap(p[player0], p[player1]);
}

struct PositionReorientation
{
	Reorientation reorientation;
	bool swap_players;
	bool mirror; // not done as part of Reorientation because requires swapping left/right limbs

	PositionReorientation(Reorientation r = {{0,0,0},0}, bool sp = false, bool m = false)
		: reorientation(r), swap_players(sp), mirror(m) {} // ugh

	Position operator()(Position p) const
	{
		p = apply(reorientation, p);
		if (mirror) p = GrappleMap::mirror(p);
		if (swap_players) GrappleMap::swap_players(p);
		return p;
	}

	V3 operator()(V3 v) const
	{
		v = apply(reorientation, v);
		if (mirror) v = GrappleMap::mirror(v);
		return v;
	}
};

inline ostream & operator<<(ostream & o, PositionReorientation const & r)
{
	return o
		<< "{" << r.reorientation
		<< ", swap_players=" << r.swap_players
		<< ", mirror=" << r.mirror
		<< '}';
}

inline PlayerJoint apply(PositionReorientation const & r, PlayerJoint pj)
{
	if (r.mirror) pj.joint = mirror(pj.joint);

	if (r.swap_players) pj.player = opponent(pj.player);

	return pj;
}

inline V3 apply(PositionReorientation const & r, Position const & p, PlayerJoint const j)
{
	V3 v = apply(r.reorientation, p[apply(r, j)]);

	if (r.mirror) v = mirror(v);

	assert(distanceSquared(v, r(p)[j]) < 0.001);

	return v;
}

PositionReorientation inverse(PositionReorientation);
PositionReorientation compose(PositionReorientation, PositionReorientation);

optional<PositionReorientation> is_reoriented(Position const &, Position);

inline bool operator==(PositionReorientation const & a, PositionReorientation const & b)
{
	return a.reorientation == b.reorientation && a.swap_players == b.swap_players && a.mirror == b.mirror;
}

inline V2 heading(Position const & p) // formalized
{
	return xz(p[player1][Core] - p[player0][Core]);
}

inline double normalRotation(Position const & p) // formalized
{
	return -angle(heading(p));
}

inline V2 center(Position const & p) // formalized
{
	return xz(between(p[player0][Core], p[player1][Core]));
}

template<typename F>
Position mapCoords(Position p, F f) // formalized
{
	foreach (j : playerJoints) p[j] = f(p[j]);
	return p;
}

inline Position rotate(double const a, Position const & p) // formalized
{
	return mapCoords(p, [a](V3 v){ return yrot(a) * v; });
}

inline Position translate(V3 const off, Position const & p)
{
	return mapCoords(p, [off](V3 v){ return v + off; });
}

inline V3 normalTranslation(Position const & p) { return y0(-center(p)); } // formalized

inline Position translateNormal(Position const & p)
{
	return translate(normalTranslation(p), p);
}

PositionReorientation canonical_reorientation_without_mirror(Position const &);
PositionReorientation canonical_reorientation_with_mirror(Position const &);

Position orient_canonically_without_mirror(Position const &);
Position orient_canonically_with_mirror(Position const &);

inline V3 cameraOffsetFor(Position const & p)
{
	V3 r = between(p[player0][Core], p[player1][Core]);
	r.y = std::max(0., r.y - .5);
	return r;
}

optional<PlayerJoint> closest_joint(Position const &, V3, double max_dist);

void apply_limits(Position &);

}

#endif
