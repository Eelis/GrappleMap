#ifndef GRAPPLEMAP_PLAYERS_HPP
#define GRAPPLEMAP_PLAYERS_HPP

#include "indexed.hpp"

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

constexpr PlayerNum player0 = PlayerNum{0};
constexpr PlayerNum player1 = PlayerNum{1};

inline array<PlayerNum, 2> playerNums() { return {{player0, player1}}; }

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

template<typename T>
inline bool operator==(PerPlayer<T> const & x, PerPlayer<T> const & y)
{
	return x.values == y.values;
}

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

extern array<PlayerJoint, joint_count * 2> const playerJoints;

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
		{ {{{LeftToe,      LeftHeel    }}, 0.23, {},    true }
		, {{{LeftToe,      LeftAnkle   }}, 0.18, {},    false}
		, {{{LeftHeel,     LeftAnkle   }}, 0.09, {},    false}
		, {{{LeftAnkle,    LeftKnee    }}, 0.43, 0.055, true }
		, {{{LeftKnee,     LeftHip     }}, 0.43, 0.085, true }
		, {{{LeftHip,      Core        }}, 0.27, {},    false}
		, {{{Core,         LeftShoulder}}, 0.37, {},    false}
		, {{{LeftShoulder, LeftElbow   }}, 0.29, {},    true }
		, {{{LeftElbow,    LeftWrist   }}, 0.26, 0.03,  true }
		, {{{LeftWrist,    LeftHand    }}, 0.08, {},    true }
		, {{{LeftHand,     LeftFingers }}, 0.08, {},    true }
		, {{{LeftWrist,    LeftFingers }}, 0.14, {},    false}

		, {{{RightToe,      RightHeel    }}, 0.23, {},    true }
		, {{{RightToe,      RightAnkle   }}, 0.18, {},    false}
		, {{{RightHeel,     RightAnkle   }}, 0.09, {},    false}
		, {{{RightAnkle,    RightKnee    }}, 0.43, 0.055, true }
		, {{{RightKnee,     RightHip     }}, 0.43, 0.085, true }
		, {{{RightHip,      Core         }}, 0.27, {},    false}
		, {{{Core,          RightShoulder}}, 0.37, {},    false}
		, {{{RightShoulder, RightElbow   }}, 0.29, {},    true }
		, {{{RightElbow,    RightWrist   }}, 0.27, 0.03,  true }
		, {{{RightWrist,    RightHand    }}, 0.08, {},    true }
		, {{{RightHand,     RightFingers }}, 0.08, {},    true }
		, {{{RightWrist,    RightFingers }}, 0.14, {},    false}

		//, {{LeftShoulder, RightShoulder}, 0.34, 0.1, false}
		, {{{LeftHip, RightHip}}, 0.23, {},  false}

		, {{{LeftShoulder, Neck}}, 0.175, {}, false}
		, {{{RightShoulder, Neck}}, 0.175, {}, false}
		, {{{Neck, Head}}, 0.165, 0.05, true}

		};

		// TODO: replace with something less stupid once I upgrade to gcc that supports 14/17

	return a;
}

using Player = PerJoint<V3>;

}

#endif
