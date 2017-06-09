#ifndef GRAPPLEMAP_REORIENTED_HPP
#define GRAPPLEMAP_REORIENTED_HPP

#include "positions.hpp"
#include "locations.hpp"

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

	template<typename T>
	inline Reoriented<T> operator*(T x, PositionReorientation r)
	{
		return {x, r};
	}

	using ReorientedNode = Reoriented<NodeNum>;
	using ReorientedSegment = Reoriented<SegmentInSequence>;

	inline Reoriented<SegmentInSequence> segment(Reoriented<Location> const & l)
	{
		return {l->segment, l.reorientation};
	}

	inline Reoriented<SeqNum> sequence(Reoriented<SegmentInSequence> const & s)
	{
		return {s->sequence, s.reorientation};
	}

	inline Reoriented<SeqNum> sequence(Reoriented<PositionInSequence> const & s)
	{
		return {s->sequence, s.reorientation};
	}

	inline Reoriented<SeqNum> sequence(Reoriented<Location> const & l)
	{
		return sequence(segment(l));
	}

	inline Reoriented<Location> loc(Reoriented<SegmentInSequence> const & s, double const c)
	{
		return Location{*s, c} * s.reorientation;
	}

	inline Reoriented<Location> loc(Reoriented<Reversible<SegmentInSequence>> const & s, double const c)
	{
		return loc(**s * s.reorientation, s->reverse ? (1-c) : c);
	}

	inline Reoriented<Step> nonreversed(Reoriented<SeqNum> const & s)
	{
		return {nonreversed(*s), s.reorientation};
	}

	inline Reoriented<Step> reversed(Reoriented<SeqNum> const & s)
	{
		return {reversed(*s), s.reorientation};
	}

	inline Reoriented<Location> from_loc(Reoriented<SegmentInSequence> const & s) { return loc(s, 0); }
	inline Reoriented<Location> to_loc(Reoriented<SegmentInSequence> const & s) { return loc(s, 1); }

	inline Reoriented<Location> from_loc(Reoriented<Reversible<SegmentInSequence>> const & s)
	{ return loc(s, 0); }

	inline Reoriented<Location> to_loc(Reoriented<Reversible<SegmentInSequence>> const & s)
	{ return loc(s, 1); }

	template<typename T>
	Reoriented<T> forget_direction(Reoriented<Reversible<T>> const & s)
	{
		return **s * s.reorientation;
	}

	inline Reoriented<PositionInSequence> first_pos_in(Reoriented<SeqNum> const s)
	{
		return first_pos_in(*s) * s.reorientation;
	}

	template<typename T>
	inline auto from(Reoriented<T> const & s)
	{
		return from(*s) * s.reorientation;
	}

	template<typename T>
	inline auto to(Reoriented<T> const & s)
	{
		return to(*s) * s.reorientation;
	}

	template<typename T>
	Reoriented<Reversible<T>> reverse(Reoriented<Reversible<T>> x)
	{
		*x = reverse(*x);
		return x;
	}

	inline Reoriented<PositionInSequence> operator*(Reoriented<SeqNum> const & seq, PosNum const pos)
	{
		return (*seq * pos) * seq.reorientation;
	}

	inline Reoriented<SegmentInSequence> operator*(Reoriented<SeqNum> const & seq, SegmentNum const seg)
	{
		return (*seq * seg) * seq.reorientation;
	}

	inline optional<Reoriented<PositionInSequence>> position(Reoriented<Location> const & l)
	{
		if (auto x = position(*l)) return *x * l.reorientation;
		return boost::none;
	}

	template <typename T>
	optional<Reoriented<T>> prev(Reoriented<T> const & s)
	{
		if (auto x = prev(*s)) return *x * s.reorientation;
		return boost::none;
	}
}

#endif
