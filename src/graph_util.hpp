#ifndef GRAPPLEMAP_GRAPH_UTIL_HPP
#define GRAPPLEMAP_GRAPH_UTIL_HPP

#include "graph.hpp"
#include <map>
#include <boost/range/counting_range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace GrappleMap {

using boost::adaptors::transformed;

inline NodeNum::range nodenums(Graph const & g) { return {NodeNum{0}, NodeNum{g.num_nodes()}}; }
inline SeqNum::range seqnums(Graph const & g) { return {SeqNum{0}, SeqNum{g.num_sequences()}}; }

SeqNum insert(Graph &, Sequence const &);
optional<SeqNum> erase_sequence(Graph &, SeqNum);
void split_at(Graph &, PositionInSequence);

// first/last/next/prev/end/from/to

inline Reoriented<Location> from_loc(Reoriented<SegmentInSequence> const & s) { return loc(s, 0); }
inline Reoriented<Location> to_loc(Reoriented<SegmentInSequence> const & s) { return loc(s, 1); }

inline Reoriented<Location> from_loc(Reoriented<Reversible<SegmentInSequence>> const & s)
{ return loc(s, 0); }

inline Reoriented<Location> to_loc(Reoriented<Reversible<SegmentInSequence>> const & s)
{ return loc(s, 1); }

inline PosNum end(Sequence const & s)
{ return {PosNum::underlying_type(s.positions.size())}; }

inline PosNum last_pos(Sequence const & s)
{
	return {PosNum::underlying_type(s.positions.size() - 1)};
}

inline PositionInSequence first_pos_in(SeqNum const s)
{
	return {s, 0};
}

inline Reoriented<PositionInSequence> first_pos_in(Reoriented<SeqNum> const s)
{
	return {first_pos_in(*s), s.reorientation};
}

inline PositionInSequence last_pos_in(SeqNum const s, Graph const & g)
{
	return {s, last_pos(g[s])};
}

template<typename T>
Reoriented<PositionInSequence> last_pos_in(Reoriented<T> const s, Graph const & g)
{
	return {last_pos_in(*s, g), s.reorientation};
}

template<typename T>
inline Reoriented<T> forget_direction(Reoriented<Reversible<T>> const & s)
{
	return {**s, s.reorientation};
}

template<typename T>
auto first_pos_in(Reversible<T> const s, Graph const & g)
{
	return s.reverse ? last_pos_in(*s, g) : first_pos_in(*s);
}

template<typename T>
Reoriented<PositionInSequence> first_pos_in(Reoriented<T> const s, Graph const & g)
{
	return {first_pos_in(*s, g), s.reorientation};
}

inline PosNum::range posnums(Graph const & g, SeqNum const s)
{
	return {PosNum{0}, end(g[s])};
}

inline auto positions(Graph const & g, SeqNum const s)
	// returns a range of PosNum
{
	return posnums(g, s) | transformed(
		[s](PosNum p){ return PositionInSequence{s, p}; });
}

inline auto positions(Graph const & g, Reoriented<SeqNum> const s)
	// returns a range of Reoriented<PositionInSequence>
{
	return positions(g, *s) | transformed(
		[s](PositionInSequence const pis)
		{ return Reoriented<PositionInSequence>{pis, s.reorientation}; });
}

inline SegmentInSequence first_segment(SeqNum const s)
{
	return {s, 0};
}

inline SegmentNum last_segment(Sequence const & s)
{
	return {SegmentNum::underlying_type(s.positions.size() - 2)};
		// e.g. 3 positions means 2 segments, so last segment index is 1
}

inline SegmentInSequence last_segment(SeqNum const s, Graph const & g)
{
	return {s, last_segment(g[s])};
}

inline Reversible<SegmentInSequence> first_segment(Reversible<SeqNum> const & s, Graph const & g)
{
	return {s.reverse ? last_segment(*s, g) : first_segment(*s), s.reverse};
}

inline Reversible<SegmentInSequence> last_segment(Reversible<SeqNum> const & s, Graph const & g)
{
	return {s.reverse ? first_segment(*s) : last_segment(*s, g), s.reverse};
}

template <typename T>
auto last_segment(Reoriented<T> const & s, Graph const & g)
	-> Reoriented<decltype(last_segment(*s, g))>
{
	return {last_segment(*s, g), s.reorientation};
}

template <typename T>
auto first_segment(Reoriented<T> const & s, Graph const & g)
	-> Reoriented<decltype(first_segment(*s, g))>
{
	return {first_segment(*s, g), s.reorientation};
}

template <typename T>
optional<Reoriented<T>> prev(Reoriented<T> const & s)
{
	if (auto x = prev(*s)) return Reoriented<T>{*x, s.reorientation};
	return boost::none;
}

template <typename T>
optional<Reoriented<T>> next(Reoriented<T> const & s, Graph const & g)
{
	if (auto x = next(*s, g)) return Reoriented<T>{*x, s.reorientation};
	return boost::none;
}

inline optional<PositionInSequence> prev(PositionInSequence const pis)
{
	if (auto pp = prev(pis.position)) return PositionInSequence{pis.sequence, *pp};
	return boost::none;
}

inline optional<PositionInSequence> next(PositionInSequence const pis, Graph const & g)
{
	if (pis.position == last_pos(g[pis.sequence])) return none;
	return PositionInSequence{pis.sequence, next(pis.position)};
}

inline optional<SegmentInSequence> prev(SegmentInSequence const s)
{
	if (auto ps = prev(s.segment)) return SegmentInSequence{s.sequence, *ps};
	else return boost::none;
}

inline SegmentInSequence next(SegmentInSequence const s)
{
	return {s.sequence, next(s.segment)};
}

inline PosNum from(SegmentNum const s) { return {s.index}; }
inline PosNum to(SegmentNum const s) { return {PosNum::underlying_type(s.index + 1)}; }

inline SegmentNum segment_from(PosNum const n) { return {n.index}; }
inline SegmentNum segment_to(PosNum const n) { return {SegmentNum::underlying_type(n.index - 1)}; }

inline SegmentInSequence segment_from(PositionInSequence const n)
{ return {n.sequence, segment_from(n.position)}; }

inline SegmentInSequence segment_to(PositionInSequence const n)
{ return {n.sequence, segment_to(n.position)}; }

inline PositionInSequence from(SegmentInSequence const s) { return {s.sequence, from(s.segment)}; }
inline PositionInSequence to(SegmentInSequence const s) { return {s.sequence, to(s.segment)}; }

template<typename T>
Reversible<T> reverse(Reversible<T> x)
{
	x.reverse = !x.reverse;
	return x;
}

inline Reoriented<NodeNum> const & from(Graph const & g, Reversible<SeqNum> const s)
{
	return s.reverse ? g[*s].to : g[*s].from;
}

inline Reoriented<NodeNum> const & to(Graph const & g, Reversible<SeqNum> const s)
{
	return from(g, reverse(s));
}

inline Reoriented<NodeNum> from(Reoriented<SeqNum> const & s, Graph const & g)
{
	ReorientedNode const & n = g[*s].from;
	return {*n, compose(n.reorientation, s.reorientation)};
}

inline Reoriented<NodeNum> to(Reoriented<SeqNum> const & s, Graph const & g)
{
	Reoriented<NodeNum> const & n = g[*s].to;
	return {*n, compose(n.reorientation, s.reorientation)};
}

inline Reoriented<PositionInSequence> from_pos(Reoriented<SegmentInSequence> const & s)
{
	return {from(*s), s.reorientation};
}

inline Reoriented<PositionInSequence> to_pos(Reoriented<SegmentInSequence> const & s)
{
	return {to(*s), s.reorientation};
}

template<typename T>
Reoriented<Reversible<T>> reverse(Reoriented<Reversible<T>> x)
{
	*x = reverse(*x);
	return x;
}

inline Reoriented<NodeNum> from(Reoriented<Reversible<SeqNum>> const & s, Graph const & g)
{
	return s->reverse
		? to(forget_direction(s), g)
		: from(forget_direction(s), g);
}

inline Reoriented<NodeNum> to(Reoriented<Reversible<SeqNum>> const & s, Graph const & g)
{
	return from(reverse(s), g);
}

inline Reoriented<PositionInSequence> operator*(Reoriented<SeqNum> const & seq, PosNum const pos)
{
	return {PositionInSequence{*seq, pos}, seq.reorientation};
}

inline Reoriented<SegmentInSequence> operator*(Reoriented<SeqNum> const & seq, SegmentNum const seg)
{
	return {SegmentInSequence{*seq, seg}, seq.reorientation};
}

inline SegmentNum::range segments(Sequence const & s)
{
	return {SegmentNum{0}, SegmentNum{SegmentNum::underlying_type(s.positions.size() - 1)}};
}

inline PosNum::range positions(Sequence const & s)
{
	return {PosNum{0}, PosNum{PosNum::underlying_type(s.positions.size())}};
}

inline auto positions(Reoriented<SeqNum> const & s, Graph const & g)
{
	return positions(g[*s])
		| boost::adaptors::transformed([s](PosNum const p){ return s * p; });
}

// in/out

Reoriented<Reversible<SeqNum>>
	gp_connect(Reoriented<NodeNum> const &, Reversible<SeqNum>, Graph const &);

inline auto connector(Reoriented<NodeNum> const n, Graph const & g)
{
	return boost::adaptors::transformed(
		[&g, n](Reversible<SeqNum> const s) { return gp_connect(n, s, g); });
}

inline auto in_sequences(Reoriented<NodeNum> const & n, Graph const & g)
{
	return g[*n].in | connector(n, g);
}

inline auto out_sequences(Reoriented<NodeNum> const & n, Graph const & g)
{
	return g[*n].out | connector(n, g);
}

// comparison

inline bool operator==(Step const a, Step const b)
{
	return *a == *b && a.reverse == b.reverse;
}

inline bool operator<(Step const a, Step const b)
{
	return std::tie(*a, a.reverse) < std::tie(*b, b.reverse);
}

inline auto in_segments(Reoriented<NodeNum> const & n, Graph const & g)
	// returns a range of Reoriented<Reversible<SegmentInSequence>>
{
	return in_sequences(n, g) | boost::adaptors::transformed(
		[&](Reoriented<Reversible<SeqNum>> const & s)
		{ return last_segment(s, g); });
}

inline auto out_segments(ReorientedNode const & n, Graph const & g)
	// returns a range of Reoriented<Reversible<SegmentInSequence>>
{
	return out_sequences(n, g) | boost::adaptors::transformed(
		[&](Reoriented<Reversible<SeqNum>> const & s)
		{ return first_segment(s, g); });
}

// at

inline Position const & at(PositionInSequence const i, Graph const & g)
{
	return g[i.sequence][i.position];
}

inline Position at(Location const & l, Graph const & g)
{
	return between(at(from(l.segment), g), at(to(l.segment), g), l.howFar);
}

inline Position at(Reoriented<Location> const & l, Graph const & g)
{
	return l.reorientation(at(*l, g));
}

inline Position at(Reoriented<PositionInSequence> const & s, Graph const & g)
{
	return s.reorientation(at(*s, g));
}

inline V3 at(Reoriented<PositionInSequence> const & s, PlayerJoint const j, Graph const & g)
{
	return apply(s.reorientation, at(*s, g), j);
}

// misc

vector<Reoriented<SegmentInSequence>>
	neighbours(Reoriented<SegmentInSequence> const &, Graph const &, bool open);

inline optional<ReorientedNode> node(Graph const & g, PositionInSequence const pis)
{
	if (pis.position.index == 0) return g[pis.sequence].from;
	if (!next(pis, g)) return g[pis.sequence].to;
	return none;
}

inline void replace(Graph & graph, PositionInSequence const pis, PlayerJoint const j, V3 const v, bool const local)
{
	Position p = at(pis, graph);
	p[j] = v;
	graph.replace(pis, p, local);
}

pair<vector<Position>, ReorientedNode> follow(Graph const &, ReorientedNode const &, SeqNum, unsigned frames_per_pos);
ReorientedNode follow(Graph const &, ReorientedNode const &, SeqNum);
NodeNum follow(Graph const &, NodeNum, SeqNum);

bool connected(Graph const &, NodeNum, NodeNum);

inline optional<NodeNum> node_at(Graph const & g, PositionInSequence const pis)
{
	if (pis == first_pos_in(pis.sequence)) return *g[pis.sequence].from;
	if (pis == last_pos_in(pis.sequence, g)) return *g[pis.sequence].to;
	return boost::none;
}

set<NodeNum> nodes_around(Graph const &, set<NodeNum> const &, unsigned depth = 1);

set<NodeNum> grow(Graph const &, set<NodeNum>, unsigned depth);

optional<SeqNum> seq_by_arg(Graph const &, string const & arg);
optional<NodeNum> node_by_arg(Graph const &, string const & arg);

inline bool is_sweep(Graph const & g, SeqNum const s)
{
	return g[s].from.reorientation.swap_players != g[s].to.reorientation.swap_players;
}

inline optional<PositionInSequence> position(Location const & l)
{
	if (l.howFar == 0) return from(l.segment);
	if (l.howFar == 1) return to(l.segment);
	return boost::none;
}

inline optional<Reoriented<PositionInSequence>> position(Reoriented<Location> const & l)
{
	if (auto x = position(*l))
		return Reoriented<PositionInSequence>{*x, l.reorientation};

	return boost::none;
}

inline auto inout_sequences(Reoriented<NodeNum> const & n, Graph const & g)
{
	return g[*n].in_out | connector(n, g);
}

inline auto joint_positions(Reoriented<SeqNum> const & s, PlayerJoint const j, Graph const & g)
{
	return positions(s, g) | boost::adaptors::transformed(
		[&g, j](Reoriented<PositionInSequence> const & p) { return at(p, j, g); });
}

}

#endif
