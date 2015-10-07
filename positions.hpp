#ifndef JIUJITSUMAPPER_POSITIONS_HPP
#define JIUJITSUMAPPER_POSITIONS_HPP

#include "math.hpp"
#include <array>
#include <cmath>

#include <boost/optional.hpp>

#define JOINTS \
	LeftToe, RightToe, \
	LeftHeel, RightHeel, \
	LeftAnkle, RightAnkle, \
	LeftKnee, RightKnee, \
	LeftHip, RightHip, \
	LeftShoulder, RightShoulder, \
	LeftElbow, RightElbow, \
	LeftWrist, RightWrist, \
	LeftHand, RightHand, \
	LeftFingers, RightFingers, \
	Core, Neck, Head

enum Joint: uint32_t { JOINTS };

constexpr Joint joints[] = { JOINTS };

#undef JOINTS

constexpr uint32_t joint_count = sizeof(joints) / sizeof(Joint);

struct PlayerJoint { unsigned player; Joint joint; };

inline bool operator==(PlayerJoint a, PlayerJoint b)
{
	return a.player == b.player && a.joint == b.joint;
}

template<typename T> using PerPlayer = std::array<T, 2>;
template<typename T> using PerJoint = std::array<T, joint_count>;

struct JointDef { Joint joint; double radius; bool draggable; };

PerJoint<JointDef> jointDefs =
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

template<typename T>
struct PerPlayerJoint: PerPlayer<PerJoint<T>>
{
	using PerPlayer<PerJoint<T>>::operator[];

	T & operator[](PlayerJoint i) { return this->operator[](i.player)[i.joint]; }
	T const & operator[](PlayerJoint i) const { return operator[](i.player)[i.joint]; }
};

using Position = PerPlayerJoint<V3>;

struct Sequence
{
	std::string description;
	std::vector<Position> positions; // invariant: .size()>=2
};

using PosNum = unsigned;

inline PosNum end(Sequence const & seq) { return seq.positions.size(); }

inline std::array<PlayerJoint, joint_count * 2> make_playerJoints()
{
	std::array<PlayerJoint, joint_count * 2> r;
	unsigned i = 0;
	for (unsigned player = 0; player != 2; ++player)
		for (auto j : joints)
			r[i++] = {player, j};
	return r;
}

const auto playerJoints = make_playerJoints();

Position operator+(Position r, V3 const off)
{
	for (auto j : playerJoints) r[j] += off;
	return r;
}

inline Position operator-(Position const & p, V3 const off)
{
	return p + -off;
}

struct PlayerDef { V3 color; };

PerPlayer<PlayerDef> playerDefs = {{ {red}, {blue} }};

struct Segment
{
	std::array<Joint, 2> ends;
	double length; // in meters
	bool visible;
};

const Segment segments[] =
	{ {{LeftToe, LeftHeel}, 0.23, true}
	, {{LeftToe, LeftAnkle}, 0.18, true}
	, {{LeftHeel, LeftAnkle}, 0.09, true}
	, {{LeftAnkle, LeftKnee}, 0.42, true}
	, {{LeftKnee, LeftHip}, 0.47, true}
	, {{LeftHip, Core}, 0.28, true}
	, {{Core, LeftShoulder}, 0.38, true}
	, {{LeftShoulder, LeftElbow}, 0.29, true}
	, {{LeftElbow, LeftWrist}, 0.26, true}
	, {{LeftWrist, LeftHand}, 0.08, true}
	, {{LeftHand, LeftFingers}, 0.08, true}
	, {{LeftWrist, LeftFingers}, 0.14, false}

	, {{RightToe, RightHeel}, 0.23, true}
	, {{RightToe, RightAnkle}, 0.18, true}
	, {{RightHeel, RightAnkle}, 0.09, true}
	, {{RightAnkle, RightKnee}, 0.42, true}
	, {{RightKnee, RightHip}, 0.47, true}
	, {{RightHip, Core}, 0.28, true}
	, {{Core, RightShoulder}, 0.38, true}
	, {{RightShoulder, RightElbow}, 0.29, true}
	, {{RightElbow, RightWrist}, 0.26, true}
	, {{RightWrist, RightHand}, 0.08, true}
	, {{RightHand, RightFingers}, 0.08, true}
	, {{RightWrist, RightFingers}, 0.14, false}

	, {{LeftShoulder, RightShoulder}, 0.4, false}
	, {{LeftHip, RightHip}, 0.25, false}

	, {{LeftShoulder, Neck}, 0.23, true}
	, {{RightShoulder, Neck}, 0.23, true}
	, {{Neck, Head}, 0.15, false}
	};

using Player = PerJoint<V3>;

inline Player spring(Player const & p, boost::optional<Joint> fixed_joint = boost::none)
{
	Player r = p;

	for (auto && j : joints)
	{
		if (j == fixed_joint) continue;

		auto f = [](double distance)
			{
				return std::max(-.3, std::min(.3, distance / 3 + distance * distance * distance));
			};

		for (auto && s : segments)
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

inline void spring(Position & pos, boost::optional<PlayerJoint> j = boost::none)
{
	for (unsigned player = 0; player != 2; ++player)
	{
		boost::optional<Joint> fixed_joint;
		if (j && j->player == player) fixed_joint = j->joint;

		pos[player] = spring(pos[player], fixed_joint);
	}
}

inline Position between(Position const & a, Position const & b, double s = 0.5 /* [0,1] */)
{
	Position r;
	for (auto j : playerJoints) r[j] = a[j] + (b[j] - a[j]) * s;
	return r;
}

bool basicallySame(Position const & a, Position const & b)
{
	double u = 0;
	for (auto j : playerJoints) u += distanceSquared(a[j], b[j]);
	return u < 0.03;
}

using SeqNum = unsigned;

struct PositionInSequence
{
	SeqNum sequence;
	PosNum position;
};

PositionInSequence prev(PositionInSequence const pis)
{
	assert(pis.position != 0);
	return {pis.sequence, pis.position - 1};
}

PositionInSequence next(PositionInSequence const pis)
{
	return {pis.sequence, pis.position + 1};
}

std::ostream & operator<<(std::ostream & o, PositionInSequence const pis)
{
	return o << "{" << pis.sequence << ", " << pis.position << "}";
}

bool operator==(PositionInSequence const & a, PositionInSequence const & b)
{
	return a.sequence == b.sequence && a.position == b.position;
}

Position apply(Reorientation const & r, Position p)
{
	for (auto j : playerJoints) p[j] = apply(r, p[j]);
	return p;
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

#endif
