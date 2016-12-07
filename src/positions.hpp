#ifndef GRAPPLEMAP_POSITIONS_HPP
#define GRAPPLEMAP_POSITIONS_HPP

#include "math.hpp"
#include "util.hpp"
#include <array>
#include <cmath>
#include <iostream>

namespace GrappleMap {

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



enum class Indexed { sequence, segment, node, position, player };

template <Indexed, typename T>
struct Index
{
	using underlying_type = T;
	T index;
};

template <Indexed i, typename T>
bool operator==(Index<i, T> a, Index<i, T> b) { return a.index == b.index; }

template <Indexed i, typename T>
bool operator!=(Index<i, T> a, Index<i, T> b) { return a.index != b.index; }

template <Indexed i, typename T>
bool operator<(Index<i, T> a, Index<i, T> b) { return a.index < b.index; }

template <Indexed i, typename T>
Index<i, T> & operator++(Index<i, T> & x)
{ ++x.index; return x; }

template <Indexed i, typename T>
optional<Index<i, T>> prev(Index<i, T> x)
{
	if (x.index == 0) return boost::none;
	return Index<i, T>{T(x.index - 1)};
}

template <Indexed i, typename T>
Index<i, T> next(Index<i, T> x)
{
	return Index<i, T>{T(x.index + 1)};
}

template <Indexed i, typename T>
std::ostream & operator<<(std::ostream & o, Index<i, T> const x)
{
	return o << int64_t(x.index);
}

using SeqNum = Index<Indexed::sequence, uint_fast32_t>;
using SegmentNum = Index<Indexed::segment, uint_fast8_t>;
using NodeNum = Index<Indexed::node, uint16_t>;
using PosNum = Index<Indexed::position, uint_fast8_t>;
using PlayerNum = Index<Indexed::player, uint_fast8_t>;

constexpr PlayerNum player0 = PlayerNum{0};
constexpr PlayerNum player1 = PlayerNum{1};

inline array<PlayerNum, 2> playerNums() { return {player0, player1}; }

inline char playerCode(PlayerNum const p){ return "tb"[p.index]; }

inline PlayerNum opponent(PlayerNum const p) { return {PlayerNum::underlying_type(1 - p.index)}; }

struct PlayerJoint { PlayerNum player; Joint joint; };

inline bool operator==(PlayerJoint a, PlayerJoint b)
{
	return a.player == b.player && a.joint == b.joint;
}

inline std::ostream & operator<<(std::ostream & o, PlayerJoint const pj)
{
	return o << "player " << pj.player << "'s " << to_string(pj.joint);
}

template<typename T>
struct PerPlayer
{
	array<T, 2> values;

	T & operator[](PlayerNum n) { return values[n.index]; }
	T const & operator[](PlayerNum n) const { return values[n.index]; }
};

template<typename T> using PerJoint = array<T, joint_count>;

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

	T & operator[](PlayerJoint i) { return (*this)[i.player][i.joint]; }
	T const & operator[](PlayerJoint i) const { return (*this)[i.player][i.joint]; }
};

using Position = PerPlayerJoint<V3>;

struct Sequence
{
	vector<string> description;
	vector<Position> positions; // invariant: .size()>=2
	optional<unsigned> line_nr;
	bool detailed;

	Position const & operator[](PosNum const n) const { return positions[n.index]; }
};

extern array<PlayerJoint, joint_count * 2> const playerJoints;

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

struct PlayerDef { V3 color; };

extern PerPlayer<PlayerDef> const playerDefs;

struct Limb
{
	array<Joint, 2> ends;
	double length;
	optional<double> midpointRadius; // in meters
	bool visible;
};

inline auto & limbs()
{
	static const Limb a[] =
		{ {{LeftToe,      LeftHeel    }, 0.23, {},    true }
		, {{LeftToe,      LeftAnkle   }, 0.18, {},    false}
		, {{LeftHeel,     LeftAnkle   }, 0.09, {},    false}
		, {{LeftAnkle,    LeftKnee    }, 0.43, 0.055, true }
		, {{LeftKnee,     LeftHip     }, 0.43, 0.085, true }
		, {{LeftHip,      Core        }, 0.27, {},    false}
		, {{Core,         LeftShoulder}, 0.37, {},    false}
		, {{LeftShoulder, LeftElbow   }, 0.29, {},    true }
		, {{LeftElbow,    LeftWrist   }, 0.26, 0.03,  true }
		, {{LeftWrist,    LeftHand    }, 0.08, {},    true }
		, {{LeftHand,     LeftFingers }, 0.08, {},    true }
		, {{LeftWrist,    LeftFingers }, 0.14, {},    false}

		, {{RightToe,      RightHeel    }, 0.23, {},    true }
		, {{RightToe,      RightAnkle   }, 0.18, {},    false}
		, {{RightHeel,     RightAnkle   }, 0.09, {},    false}
		, {{RightAnkle,    RightKnee    }, 0.43, 0.055, true }
		, {{RightKnee,     RightHip     }, 0.43, 0.085, true }
		, {{RightHip,      Core         }, 0.27, {},    false}
		, {{Core,          RightShoulder}, 0.37, {},    false}
		, {{RightShoulder, RightElbow   }, 0.29, {},    true }
		, {{RightElbow,    RightWrist   }, 0.27, 0.03,  true }
		, {{RightWrist,    RightHand    }, 0.08, {},    true }
		, {{RightHand,     RightFingers }, 0.08, {},    true }
		, {{RightWrist,    RightFingers }, 0.14, {},    false}

		//, {{LeftShoulder, RightShoulder}, 0.34, 0.1, false}
		, {{LeftHip, RightHip}, 0.23, {},  false}

		, {{LeftShoulder, Neck}, 0.175, {}, false}
		, {{RightShoulder, Neck}, 0.175, {}, false}
		, {{Neck, Head}, 0.165, 0.05, true}

		};

		// TODO: replace with something less stupid once I upgrade to gcc that supports 14/17

	return a;
}

using Player = PerJoint<V3>;

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

struct PositionInSequence
{
	SeqNum sequence;
	PosNum position;
};


template<typename T>
struct Reversible
{
	T value;

	bool reverse;

	T * operator->() { return &value; }
	T const * operator->() const { return &value; }

	T & operator*() { return value; }
	T const & operator*() const { return value; }
};

using Step = Reversible<SeqNum>;

inline Step forwardStep(SeqNum const s) { return {s, false}; }
inline Step backStep(SeqNum const s) { return {s, true}; }

struct SegmentInSequence
{
	SeqNum sequence;
	SegmentNum segment;
};

struct Location
{
	SegmentInSequence segment;
	double howFar; // [0..1] how far along segment
};

inline std::ostream & operator<<(std::ostream & o, NodeNum const n)
{
	return o << "node" << n;
}

inline bool operator==(SegmentInSequence const a, SegmentInSequence const b)
{
	return a.sequence == b.sequence && a.segment == b.segment;
}

inline bool operator!=(SegmentInSequence const a, SegmentInSequence const b) { return !(a == b); }

inline bool operator<(SegmentInSequence const a, SegmentInSequence const b)
{
	return std::make_tuple(a.sequence, a.segment) < std::make_tuple(b.sequence, b.segment);
}

inline std::ostream & operator<<(std::ostream & o, PositionInSequence const pis)
{
	return o << "{" << pis.sequence << ", " << pis.position << "}";
}

inline bool operator==(PositionInSequence const & a, PositionInSequence const & b)
{
	return a.sequence == b.sequence && a.position == b.position;
}

inline bool operator!=(PositionInSequence const & a, PositionInSequence const & b)
{
	return !(a == b);
}

inline Position apply(Reorientation const & r, Position p)
{
	for (auto j : playerJoints) p[j] = apply(r, p[j]);
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
