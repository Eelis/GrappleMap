#include "positions.hpp"
#include "util.hpp"
#include "persistence.hpp"

namespace GrappleMap {

extern PerJoint<JointDef> const jointDefs =
	{{ { LeftToe, 0.025, false}
	, { RightToe, 0.025, false}
	, { LeftHeel, 0.03, false}
	, { RightHeel, 0.03, false}
	, { LeftAnkle, 0.03, true}
	, { RightAnkle, 0.03, true}
	, { LeftKnee, 0.05, true}
	, { RightKnee, 0.05, true}
	, { LeftHip, 0.09, true}
	, { RightHip, 0.09, true}
	, { LeftShoulder, 0.08, true}
	, { RightShoulder, 0.08, true}
	, { LeftElbow, 0.045, true}
	, { RightElbow, 0.045, true}
	, { LeftWrist, 0.02, false}
	, { RightWrist, 0.02, false}
	, { LeftHand, 0.02, true}
	, { RightHand, 0.02, true}
	, { LeftFingers, 0.02, false}
	, { RightFingers, 0.02, false}
	, { Core, 0.1, true}
	, { Neck, 0.05, false}
	, { Head, 0.11, true}
	}};

char const * to_string(Joint const j)
{
	switch (j)
	{
		case LeftToe: return "left toe";
		case RightToe: return "right toe";
		case LeftHeel: return "left heel";
		case RightHeel: return "right heel";
		case LeftAnkle: return "left ankle";
		case RightAnkle: return "right ankle";
		case LeftKnee: return "left knee";
		case RightKnee: return "right knee";
		case LeftHip: return "left hip";
		case RightHip: return "right hip";
		case LeftShoulder: return "left shoulder";
		case RightShoulder: return "right shoulder";
		case LeftElbow: return "left elbow";
		case RightElbow: return "right elbow";
		case LeftWrist: return "left wrist";
		case RightWrist: return "right wrist";
		case LeftHand: return "left hand";
		case RightHand: return "right hand";
		case LeftFingers: return "left fingers";
		case RightFingers: return "right fingers";
		case Core: return "core";
		case Neck: return "neck";
		case Head: return "head";

		default: return "?";
	}
}

inline array<PlayerJoint, joint_count * 2> make_playerJoints()
{
	array<PlayerJoint, joint_count * 2> r;
	unsigned i = 0;
	foreach (n : playerNums())
		foreach (j : joints)
			r[i++] = {n, j};
	return r;
}

namespace
{
	void swapLimbs(Player & p)
	{
		std::swap(p[LeftShoulder], p[RightShoulder]);
		std::swap(p[LeftHip], p[RightHip]);
		std::swap(p[LeftHand], p[RightHand]);
		std::swap(p[LeftWrist], p[RightWrist]);
		std::swap(p[LeftElbow], p[RightElbow]);
		std::swap(p[LeftFingers], p[RightFingers]);
		std::swap(p[LeftAnkle], p[RightAnkle]);
		std::swap(p[LeftToe], p[RightToe]);
		std::swap(p[LeftHeel], p[RightHeel]);
		std::swap(p[LeftKnee], p[RightKnee]);
	}

	#ifndef NDEBUG
	//Position const testPos = decodePosition("QNaAvDERaAvTO1cNx2GIczysPQbDx6FJbyysRFaYEDEjaZE1MFdvAdI5dqAjMWl1CjHwlLBPOdhyCOFkhDByM8gPGOGegWFIMVhmHVGKg3GQNygSIZGkgwHTKGhkzmJ9mzCCJKoxEqHVazAXL7azAFIMaEEAMLaEEiIzbBDzMtbDDjEYfCHAOKfKH9HzbNMXK8bMMQGHkBMGL8kBMUFKf6LMOqgzMoEMgYHLNQg4H8EIhvGCOvhGHgFJh0FYN3hQF3JnfuOnJrloMrJznEKY");
	#endif
}


PositionReorientation inverse(PositionReorientation const x) // formalized
{
	PositionReorientation r {inverse(x.reorientation), x.swap_players, x.mirror};
	if (x.mirror)
	{
		r.reorientation.angle = -r.reorientation.angle;
		r.reorientation.offset.x = -r.reorientation.offset.x;
	}

	//assert(basicallySame(r(x(testPos)), testPos));

	return r;
}

PositionReorientation compose(PositionReorientation const a, PositionReorientation const b) // todo: formalize
{
	auto br = b.reorientation;

	PositionReorientation r;

	if (a.mirror)
	{
		br.angle = -br.angle;
		br.offset.x = -br.offset.x;
	}

	r = PositionReorientation{compose(a.reorientation, br), a.swap_players != b.swap_players, a.mirror != b.mirror};

	//assert(basicallySame(b(a(testPos)), r(testPos)));

	return r;
}

Position mirror(Position p) // formalized
{
	foreach (j : playerJoints) p[j] = mirror(p[j]);
	swapLimbs(p[player0]);
	swapLimbs(p[player1]);
	return p;
}

extern array<PlayerJoint, joint_count * 2> const playerJoints = make_playerJoints();

extern PerPlayer<PlayerDef> const playerDefs = {{{ {red}, {V3{0.15,0.15,1}} }}};

Player spring(Player const & p, optional<Joint> fixed_joint)
{
	Player r = p;

	foreach (j : joints)
	{
		if (j == fixed_joint) continue;

		auto f = [](double distance)
			{
				return std::max(-.3, std::min(.3, distance / 3 + distance * distance * distance));
			};

		foreach (s : limbs())
		{
			if (s.ends[0] == j)
			{
				double force = f(s.length - distance(p[s.ends[1]], p[s.ends[0]]));
				if (std::abs(force) > 0.001)
				{
					V3 dir = normalize(p[s.ends[1]] - p[s.ends[0]]);
					r[j] -= dir * force;
				}
			}
			else if (s.ends[1] == j)
			{
				double force = f(s.length - distance(p[s.ends[1]], p[s.ends[0]]));
				if (std::abs(force) > 0.001)
				{
					V3 dir = normalize(p[s.ends[0]] - p[s.ends[1]]);
					r[j] -= dir * force;
				}
			}
		}

		r[j].y = std::max(jointDefs[j].radius, r[j].y);
	}

	return r;
}

void spring(Position & pos, optional<PlayerJoint> j)
{
	apply_limits(pos);

	foreach (player : playerNums())
	{
		optional<Joint> fixed_joint;
		if (j && j->player == player) fixed_joint = j->joint;

		pos[player] = spring(pos[player], fixed_joint);
	}
}

namespace
{
	optional<Reorientation> is_reoriented_without_mirror_and_swap(Position const & a, Position const & b)
	{
		auto const a0h = a[player0][Head];
		auto const a1h = a[player1][Head];
		auto const b0h = b[player0][Head];
		auto const b1h = b[player1][Head];

		double const angleOff = angle(xz(b1h - b0h)) - angle(xz(a1h - a0h));

		Reorientation const r{b0h - xyz(yrot(angleOff) * V4(a0h, 1)), angleOff};

		if (basicallySame(apply(r, a), b)) return r;
		else return none;
	}

	optional<PositionReorientation> is_reoriented_without_swap(Position const & a, Position const & b)
	{
		if (auto r = is_reoriented_without_mirror_and_swap(a, b))
		{
			PositionReorientation u{*r, false, false};
			assert(basicallySame(u(a), b));
			return u;
		}
		
		if (auto r = is_reoriented_without_mirror_and_swap(a, mirror(b)))
		{
			PositionReorientation u{*r, false, true};
			assert(basicallySame(u(a), b));
			return u;
		}

		return none;
	}
}

optional<PositionReorientation> is_reoriented(Position const & a, Position b)
{
	auto head2head = [](Position const & p)
		{ return distanceSquared(p[player0][Head], p[player1][Head]); };

	if (std::abs(head2head(a) - head2head(b)) > 0.05)
		return boost::none;
			// valid because the head-to-head distance is not
			// affected by position reorientations at all

	optional<PositionReorientation> r = is_reoriented_without_swap(a, b);

	#ifndef NDEBUG
		Position const c = b;
	#endif

	if (!r)
	{
		std::swap(b[player0], b[player1]);
		r = is_reoriented_without_swap(a, b);
		if (r) r->swap_players = true;
	}

	if (r) assert(basicallySame((*r)(a), c));

	return r;
}

PositionReorientation canonical_reorientation_with_mirror(Position const & p) // formalized
{
	PositionReorientation reo;
	reo.reorientation.angle = normalRotation(p);
	reo.reorientation.offset = normalTranslation(rotate(reo.reorientation.angle, p));
	reo.swap_players = false;
	reo.mirror = apply(reo.reorientation, p)[player1][Head].x >= 0;

	return reo;
}

PositionReorientation canonical_reorientation_without_mirror(Position const & p)
{
	PositionReorientation reo;
	reo.reorientation.angle = normalRotation(p);
	reo.reorientation.offset = normalTranslation(rotate(reo.reorientation.angle, p));
	reo.swap_players = false;
	reo.mirror = false;

	return reo;
}

Position orient_canonically_without_mirror(Position const & p)
{
	return canonical_reorientation_without_mirror(p)(p);
}

Position orient_canonically_with_mirror(Position const & p)
{
	return canonical_reorientation_with_mirror(p)(p);
}

pair<PlayerJoint, double> closest_joint(Position const & p, V3 const v)
{
	double d = 100000;
	PlayerJoint r;

	foreach(j : playerJoints)
	{
		double dd = distanceSquared(p[j], v);
		if (dd < d) { r = j; d = dd; }
	}

	return {r, d};
}

optional<PlayerJoint> closest_joint(Position const & p, V3 const v, double const max_dist)
{
	auto const j = closest_joint(p, v);
	if (j.second < max_dist * max_dist) return j.first;
	return boost::none;
}

void apply_limits(Position & p)
{
	foreach (j : playerJoints)
	{
		p[j].y = std::max(jointDefs[j.joint].radius, p[j].y);
		p[j].x = std::max(-2., std::min(2., p[j].x));
		p[j].z = std::max(-2., std::min(2., p[j].z));
	}
}

}
