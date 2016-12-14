#ifndef GRAPPLEMAP_VIABLES_HPP
#define GRAPPLEMAP_VIABLES_HPP

#include "reoriented.hpp"
#include <map>

namespace GrappleMap
{
	struct Graph;
	struct Camera;

	struct Viable
	{
		Reoriented<SeqNum> sequence;
		PosNum begin, end; // half-open range, never empty

		V3 beginV3, endV3;
		V2 beginxy, endxy;
	};

	struct ViablesForJoint
	{
		double total_dist;
		std::deque<Viable> viables;
		std::vector<LineSegment> segments;
	};

	using Viables = PerPlayerJoint<ViablesForJoint>;

	ViablesForJoint determineViables(
		Graph const &, Reoriented<PositionInSequence>,
		PlayerJoint, Camera const *);
}

#endif
