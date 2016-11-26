#ifndef GRAPPLEMAP_VIABLES_HPP
#define GRAPPLEMAP_VIABLES_HPP

#include "positions.hpp"
#include <map>

namespace GrappleMap
{
	struct Graph;
	struct Camera;

	struct Viable
	{
		SeqNum seqNum;
		PositionReorientation reorientation;
		double dist;
		PosNum begin, end; // half-open range, never empty
		V3 beginV3, endV3;
		V2 beginxy, endxy;
	};

	struct ViablesForJoint
	{
		double total_dist;
		std::map<SeqNum, Viable> viables;
		std::vector<LineSegment> segments;
	};

	using Viables = PerPlayerJoint<ViablesForJoint>;

	ViablesForJoint determineViables(
		Graph const &, PositionInSequence, PlayerJoint,
		bool edit_mode, Camera const &, PositionReorientation);

	struct VrViable
	{
		SeqNum seqNum;
		PositionReorientation reorientation;
		double dist;
		PosNum begin, end; // half-open range, never empty
		V3 beginV3, endV3;
	};

	using VrViablesForJoint = std::map<SeqNum, VrViable>;

	VrViablesForJoint determineVrViables(
		Graph const &, PositionInSequence, PlayerJoint,
		bool edit_mode, PositionReorientation);
}

#endif
