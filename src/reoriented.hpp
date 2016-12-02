#ifndef GRAPPLEMAP_REORIENTED_HPP
#define GRAPPLEMAP_REORIENTED_HPP

#include "positions.hpp"

namespace GrappleMap
{
	struct ReorientedNode
	{
		NodeNum node;
		PositionReorientation reorientation;
	};

	struct ReorientedSequence
	{
		SeqNum sequence;
		PositionReorientation reorientation;
	};

	struct ReorientedLocation
	{
		Location location;
		PositionReorientation reorientation;
	};

	struct ReorientedSegment
	{
		SegmentInSequence segment;
		PositionReorientation reorientation;
	};

	inline ReorientedSequence sequence(ReorientedSegment const & s)
	{
		return {s.segment.sequence, s.reorientation};
	}

	inline ReorientedSegment segment(ReorientedLocation const & l)
	{
		return {l.location.segment, l.reorientation};
	}

	inline ReorientedLocation loc(ReorientedSegment const & s, double const c)
	{
		return {{s.segment, c}, s.reorientation};
	}
}

#endif
