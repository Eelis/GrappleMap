#ifndef JIUJITSUMAPPER_POSITIONS_HPP
#define JIUJITSUMAPPER_POSITIONS_HPP

#include "math.hpp"
#include "util.hpp"
#include <array>
#include <cmath>
#include <iostream>

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

char const * to_string(Joint);

constexpr uint32_t joint_count = sizeof(joints) / sizeof(Joint);

using PlayerNum = unsigned;

inline PlayerNum opponent(PlayerNum const p) { return 1 - p; }

struct PlayerJoint { PlayerNum player; Joint joint; };

inline bool operator==(PlayerJoint a, PlayerJoint b)
{
	return a.player == b.player && a.joint == b.joint;
}

inline std::ostream & operator<<(std::ostream & o, PlayerJoint const pj)
{
	return o << "player " << pj.player << "'s " << to_string(pj.joint);
}

template<typename T> using PerPlayer = std::array<T, 2>;
template<typename T> using PerJoint = std::array<T, joint_count>;

struct JointDef { Joint joint; double radius; bool draggable; };

extern PerJoint<JointDef> const jointDefs;

inline Joint mirror(Joint const j)
{
	switch (j)
	{
		case LeftToe: return RightToe;
		case LeftHeel: return RightHeel;
		case LeftAnkle: return RightAnkle;
		case LeftKnee: return RightKnee;
		case LeftHip: return RightHip;
		case LeftShoulder: return RightShoulder;
		case LeftElbow: return RightElbow;
		case LeftWrist: return RightWrist;
		case LeftHand: return RightHand;
		case LeftFingers: return RightFingers;

		case RightToe: return LeftToe;
		case RightHeel: return LeftHeel;
		case RightAnkle: return LeftAnkle;
		case RightKnee: return LeftKnee;
		case RightHip: return LeftHip;
		case RightShoulder: return LeftShoulder;
		case RightElbow: return LeftElbow;
		case RightWrist: return LeftWrist;
		case RightHand: return LeftHand;
		case RightFingers: return LeftFingers;

		default: return j;
	}
}

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
	vector<string> description;
	vector<Position> positions; // invariant: .size()>=2
};

using PosNum = unsigned;

inline PosNum end(Sequence const & seq) { return seq.positions.size(); }

extern std::array<PlayerJoint, joint_count * 2> const playerJoints;

inline Position operator+(Position r, V3 const off)
{
	for (auto j : playerJoints) r[j] += off;
	return r;
}

inline Position operator-(Position const & p, V3 const off)
{
	return p + -off;
}

struct PlayerDef { V3 color; };

extern PerPlayer<PlayerDef> const playerDefs;

struct Segment
{
	std::array<Joint, 2> ends;
	double length, midpointRadius; // in meters
	bool visible;
};

inline auto & segments()
{
	static const Segment segments[] =
		{ {{LeftToe, LeftHeel}, 0.23, 0.025, true}
		, {{LeftToe, LeftAnkle}, 0.18, 0.025, true}
		, {{LeftHeel, LeftAnkle}, 0.09, 0.025, true}
		, {{LeftAnkle, LeftKnee}, 0.42, 0.055, true}
		, {{LeftKnee, LeftHip}, 0.44, 0.085, true}
		, {{LeftHip, Core}, 0.27, 0.1, true}
		, {{Core, LeftShoulder}, 0.37, 0.075, true}
		, {{LeftShoulder, LeftElbow}, 0.29, 0.06, true}
		, {{LeftElbow, LeftWrist}, 0.26, 0.03, true}
		, {{LeftWrist, LeftHand}, 0.08, 0.02, true}
		, {{LeftHand, LeftFingers}, 0.08, 0.02, true}
		, {{LeftWrist, LeftFingers}, 0.14, 0.02, false}

		, {{RightToe, RightHeel}, 0.23, 0.025, true}
		, {{RightToe, RightAnkle}, 0.18, 0.025, true}
		, {{RightHeel, RightAnkle}, 0.09, 0.025, true}
		, {{RightAnkle, RightKnee}, 0.42, 0.055, true}
		, {{RightKnee, RightHip}, 0.44, 0.085, true}
		, {{RightHip, Core}, 0.27, 0.1, true}
		, {{Core, RightShoulder}, 0.37, 0.075, true}
		, {{RightShoulder, RightElbow}, 0.29, 0.06, true}
		, {{RightElbow, RightWrist}, 0.27, 0.03, true}
		, {{RightWrist, RightHand}, 0.08, 0.02, true}
		, {{RightHand, RightFingers}, 0.08, 0.02, true}
		, {{RightWrist, RightFingers}, 0.14, 0.02, false}

		, {{LeftShoulder, RightShoulder}, 0.34, 0.1, false}
		, {{LeftHip, RightHip}, 0.22, 0.1,  false}

		, {{LeftShoulder, Neck}, 0.175, 0.065, true}
		, {{RightShoulder, Neck}, 0.175, 0.065, true}
		, {{Neck, Head}, 0.165, 0.05, true}

		};

		// TODO: replace with something less stupid once I upgrade to gcc that supports 14/17

	return segments;
}

using Player = PerJoint<V3>;

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

struct SeqNum { unsigned index; };

inline bool operator==(SeqNum const a, SeqNum const b) { return a.index == b.index; }
inline bool operator!=(SeqNum const a, SeqNum const b) { return a.index != b.index; }
inline bool operator<(SeqNum const a, SeqNum const b) { return a.index < b.index; }

struct PositionInSequence
{
	SeqNum sequence;
	PosNum position;
};

inline std::ostream & operator<<(std::ostream & o, PositionInSequence const pis)
{
	return o << "{" << pis.sequence.index << ", " << pis.position << "}";
}

inline bool operator==(PositionInSequence const & a, PositionInSequence const & b)
{
	return a.sequence == b.sequence && a.position == b.position;
}

inline Position apply(Reorientation const & r, Position p)
{
	for (auto j : playerJoints) p[j] = apply(r, p[j]);
	return p;
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
		if (mirror) p = ::mirror(p);
		if (swap_players) std::swap(p[0], p[1]);
		return p;
	}
};

inline std::ostream & operator<<(std::ostream & o, PositionReorientation const & r)
{
	return o
		<< "{" << r.reorientation
		<< ", swap_players=" << r.swap_players
		<< ", mirror=" << r.mirror
		<< '}';
}

inline V3 apply(PositionReorientation const & r, Position const & p, PlayerJoint j)
{
	return r(p)[j]; // todo: inefficient
}

inline PlayerJoint apply(PositionReorientation const & r, PlayerJoint pj)
{
	if (r.mirror) pj.joint = mirror(pj.joint);

	if (r.swap_players) pj.player = opponent(pj.player);

	return pj;
}

PositionReorientation inverse(PositionReorientation);

inline PositionReorientation compose(PositionReorientation const a, PositionReorientation const b)
{
	return PositionReorientation{compose(a.reorientation, b.reorientation), a.swap_players != b.swap_players, a.mirror != b.mirror};
}

optional<PositionReorientation> is_reoriented(Position const &, Position);

inline bool operator==(PositionReorientation const & a, PositionReorientation const & b)
{
	return a.reorientation == b.reorientation && a.swap_players == b.swap_players && a.mirror == b.mirror;
}

#endif
