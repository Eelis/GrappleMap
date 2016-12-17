#ifndef GRAPPLEMAP_LOCATIONS_HPP
#define GRAPPLEMAP_LOCATIONS_HPP

#include "indexed.hpp"

namespace GrappleMap {

struct PositionInSequence
{
	SeqNum sequence;
	PosNum position;
};

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

template<typename T>
Reversible<T> reverse(Reversible<T> x)
{
	x.reverse = !x.reverse;
	return x;
}

template<typename T>
bool operator==(Reversible<T> const & a, Reversible<T> const & b)
{
	return *a == *b && a.reverse == b.reverse;
}

template<typename T>
bool operator<(Reversible<T> const & a, Reversible<T> const & b)
{
	return std::tie(*a, a.reverse) < std::tie(*b, b.reverse);
}

template<typename T>
Reversible<T> nonreversed(T const x) { return {x, false}; }

template<typename T>
Reversible<T> reversed(T const x) { return {x, true}; }

using Step = Reversible<SeqNum>;

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

inline PositionInSequence operator*(SeqNum s, PosNum p) { return {s, p}; }
inline SegmentInSequence operator*(SeqNum s, SegmentNum n) { return {s, n}; }

inline optional<PositionInSequence> prev(PositionInSequence const pis)
{
	if (auto pp = prev(pis.position)) return pis.sequence * *pp;
	return boost::none;
}

inline optional<SegmentInSequence> prev(SegmentInSequence const s)
{
	if (auto ps = prev(s.segment)) return s.sequence * *ps;
	else return boost::none;
}

inline PosNum from(SegmentNum const s) { return {s.index}; }
inline PosNum to(SegmentNum const s) { return {PosNum::underlying_type(s.index + 1)}; }

inline PositionInSequence from(SegmentInSequence const s) { return s.sequence * from(s.segment); }
inline PositionInSequence to(SegmentInSequence const s) { return s.sequence * to(s.segment); }

inline PositionInSequence first_pos_in(SeqNum const s) { return s * PosNum{0}; }
inline SegmentInSequence first_segment(SeqNum const s) { return s * SegmentNum{0}; }

inline optional<PositionInSequence> position(Location const & l)
{
	if (l.howFar == 0) return from(l.segment);
	if (l.howFar == 1) return to(l.segment);
	return boost::none;
}

inline SegmentInSequence next(SegmentInSequence const s) { return s.sequence * next(s.segment); }

inline SegmentNum segment_from(PosNum const n) { return {n.index}; }
inline SegmentNum segment_to(PosNum const n) { return {SegmentNum::underlying_type(n.index - 1)}; }

inline SegmentInSequence segment_from(PositionInSequence const n)
{ return n.sequence * segment_from(n.position); }

inline SegmentInSequence segment_to(PositionInSequence const n)
{ return n.sequence * segment_to(n.position); }

}

#endif
