#ifndef GRAPPLEMAP_REORIENTED_HPP
#define GRAPPLEMAP_REORIENTED_HPP

#include "positions.hpp"

namespace GrappleMap
{
	template<typename T>
	struct Reoriented
	{
		T value;
		PositionReorientation reorientation;

		T * operator->() { return &value; }
		T const * operator->() const { return &value; }

		T & operator*() { return value; }
		T const & operator*() const { return value; }
	};

	using ReorientedNode = Reoriented<NodeNum>;
	using ReorientedSegment = Reoriented<SegmentInSequence>;

	inline Reoriented<SeqNum> sequence(Reoriented<SegmentInSequence> const & s)
	{
		return {s->sequence, s.reorientation};
	}

	inline Reoriented<SeqNum> sequence(Reoriented<PositionInSequence> const & s)
	{
		return {s->sequence, s.reorientation};
	}

	inline Reoriented<SegmentInSequence> segment(Reoriented<Location> const & l)
	{
		return {l->segment, l.reorientation};
	}

	inline Location start_loc(Reversible<SegmentInSequence> const & s)
	{
		return {*s, s.reverse ? 1.0 : 0.0};
	}

	inline Reoriented<Location> loc(Reoriented<SegmentInSequence> const & s, double const c)
	{
		return {{*s, c}, s.reorientation};
	}

	inline Reoriented<Location> loc(Reoriented<Reversible<SegmentInSequence>> const & s, double const c)
	{
		return loc(
			Reoriented<SegmentInSequence>{**s, s.reorientation},
			s->reverse ? (1-c) : c);
	}

	inline Reoriented<Step> forwardStep(Reoriented<SeqNum> const & s)
	{
		return {forwardStep(*s), s.reorientation};
	}

	inline Reoriented<Step> backStep(Reoriented<SeqNum> const & s)
	{
		return {backStep(*s), s.reorientation};
	}
}

#endif
