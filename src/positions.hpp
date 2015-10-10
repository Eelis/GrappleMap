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

char const * to_string(Joint);

constexpr uint32_t joint_count = sizeof(joints) / sizeof(Joint);

struct PlayerJoint { unsigned player; Joint joint; };

inline bool operator==(PlayerJoint a, PlayerJoint b)
{
	return a.player == b.player && a.joint == b.joint;
}

template<typename T> using PerPlayer = std::array<T, 2>;
template<typename T> using PerJoint = std::array<T, joint_count>;

struct JointDef { Joint joint; double radius; bool draggable; };

extern PerJoint<JointDef> const jointDefs;

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
	double length; // in meters
	bool visible;
};

inline auto & segments()
{
	static const Segment segments[] =
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

		// TODO: replace with something less stupid once I upgrade to gcc that supports 14/17

	return segments;
}

using Player = PerJoint<V3>;

Player spring(Player const &, boost::optional<Joint> fixed_joint = boost::none);
void spring(Position &, boost::optional<PlayerJoint> = boost::none);

inline Position between(Position const & a, Position const & b, double s = 0.5 /* [0,1] */)
{
	Position r;
	for (auto j : playerJoints) r[j] = a[j] + (b[j] - a[j]) * s;
	return r;
}

inline bool basicallySame(Position const & a, Position const & b)
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

inline std::ostream & operator<<(std::ostream & o, PositionInSequence const pis)
{
	return o << "{" << pis.sequence << ", " << pis.position << "}";
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

boost::optional<Reorientation> is_reoriented(Position const &, Position const &);

#endif
