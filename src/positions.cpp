#include "positions.hpp"
#include "util.hpp"

extern PerJoint<JointDef> const jointDefs =
	{{ { LeftToe, 0.025, false}
	, { RightToe, 0.025, false}
	, { LeftHeel, 0.03, false}
	, { RightHeel, 0.03, false}
	, { LeftAnkle, 0.03, true}
	, { RightAnkle, 0.03, true}
	, { LeftKnee, 0.05, true}
	, { RightKnee, 0.05, true}
	, { LeftHip, 0.10, true}
	, { RightHip, 0.10, true}
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
	, { Neck, 0.04, false}
	, { Head, 0.11, true}
	}};

inline std::array<PlayerJoint, joint_count * 2> make_playerJoints()
{
	std::array<PlayerJoint, joint_count * 2> r;
	unsigned i = 0;
	for (unsigned player = 0; player != 2; ++player)
		foreach (j : joints)
			r[i++] = {player, j};
	return r;
}

extern std::array<PlayerJoint, joint_count * 2> const playerJoints = make_playerJoints();

extern PerPlayer<PlayerDef> const playerDefs = {{ {red}, {blue} }};

Player spring(Player const & p, boost::optional<Joint> fixed_joint)
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

void spring(Position & pos, boost::optional<PlayerJoint> j)
{
	for (unsigned player = 0; player != 2; ++player)
	{
		boost::optional<Joint> fixed_joint;
		if (j && j->player == player) fixed_joint = j->joint;

		pos[player] = spring(pos[player], fixed_joint);
	}
}

boost::optional<Reorientation> is_reoriented(Position const & a, Position const & b)
{
	auto const a0h = a[0][Head];
	auto const a1h = a[1][Head];
	auto const b0h = b[0][Head];
	auto const b1h = b[1][Head];

	double const angleOff = angle(xz(b1h - b0h)) - angle(xz(a1h - a0h));

	Reorientation const r = { b0h - xyz(yrot(angleOff) * V4(a0h, 1)), angleOff };

	if (basicallySame(apply(r, a), b)) return r;
	else return boost::none;
}
