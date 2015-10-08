#ifndef JIUJITSUMAPPER_VIABLES_HPP
#define JIUJITSUMAPPER_VIABLES_HPP

#include "camera.hpp"
#include "positions.hpp"
#include "graph.hpp"
#include <map>

struct Viable
{
	SeqNum seqNum;
	Reorientation reorientation;
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
	bool edit_mode, Camera const &, Reorientation);

#endif
