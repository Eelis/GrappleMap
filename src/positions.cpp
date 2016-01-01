#include "positions.hpp"
#include "util.hpp"
#include "persistence.hpp"

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
	, { Core, 0.1, false}
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

inline std::array<PlayerJoint, joint_count * 2> make_playerJoints()
{
	std::array<PlayerJoint, joint_count * 2> r;
	unsigned i = 0;
	for (unsigned player = 0; player != 2; ++player)
		foreach (j : joints)
			r[i++] = {player, j};
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
	Position const testPos = decodePosition("QNaAvDERaAvTO1cNx2GIczysPQbDx6FJbyysRFaYEDEjaZE1MFdvAdI5dqAjMWl1CjHwlLBPOdhyCOFkhDByM8gPGOGegWFIMVhmHVGKg3GQNygSIZGkgwHTKGhkzmJ9mzCCJKoxEqHVazAXL7azAFIMaEEAMLaEEiIzbBDzMtbDDjEYfCHAOKfKH9HzbNMXK8bMMQGHkBMGL8kBMUFKf6LMOqgzMoEMgYHLNQg4H8EIhvGCOvhGHgFJh0FYN3hQF3JnfuOnJrloMrJznEKY");
	#endif
}


PositionReorientation inverse(PositionReorientation const x)
{
	PositionReorientation r {inverse(x.reorientation), x.swap_players, x.mirror};
	if (x.mirror)
	{
		r.reorientation.angle = -r.reorientation.angle;
		r.reorientation.offset.x = -r.reorientation.offset.x;
	}

	assert(basicallySame(r(x(testPos)), testPos));

	return r;
}

PositionReorientation compose(PositionReorientation const a, PositionReorientation const b)
{
	auto br = b.reorientation;

	PositionReorientation r;

	if (a.mirror)
	{
		br.angle = -br.angle;
		br.offset.x = -br.offset.x;
	}

	r = PositionReorientation{compose(a.reorientation, br), a.swap_players != b.swap_players, a.mirror != b.mirror};

	assert(basicallySame(b(a(testPos)), r(testPos)));

	return r;
}

Position mirror(Position p)
{
	foreach (j : playerJoints) p[j].x = -p[j].x;
	swapLimbs(p[0]);
	swapLimbs(p[1]);
	return p;
}

extern std::array<PlayerJoint, joint_count * 2> const playerJoints = make_playerJoints();

extern PerPlayer<PlayerDef> const playerDefs = {{ {red}, {V3{0.15,0.15,1}} }};

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

		foreach (s : segments())
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
	for (unsigned player = 0; player != 2; ++player)
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
		auto const a0h = a[0][Head];
		auto const a1h = a[1][Head];
		auto const b0h = b[0][Head];
		auto const b1h = b[1][Head];

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
	optional<PositionReorientation> r = is_reoriented_without_swap(a, b);

	Position const c = b;

	if (!r)
	{
		std::swap(b[0], b[1]);
		r = is_reoriented_without_swap(a, b);
		if (r) r->swap_players = true;
	}

	if (r) assert(basicallySame((*r)(a), c));

	return r;
}

PositionReorientation canonical_reorientation(Position const & p)
{
	V2 const center = xz(p[0][Core] + p[1][Core]) / 2;

	PositionReorientation reo;
	reo.reorientation.offset.x = -center.x;
	reo.reorientation.offset.z = -center.y;

	PositionReorientation r2;
	auto ding = xz(p[1][Head]) - xz(p[1][LeftToe]);
	r2.reorientation.angle = atan2(ding.x, ding.y);

	return compose(reo, r2);
}
