#ifndef GRAPPLEMAP_GRAPH_UTIL_HPP
#define GRAPPLEMAP_GRAPH_UTIL_HPP

#include "graph.hpp"
#include <map>
#include <boost/range/counting_range.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace GrappleMap {

using NodeNumIter = boost::counting_iterator<NodeNum, std::forward_iterator_tag, int32_t>;
using SeqNumIter = boost::counting_iterator<SeqNum, std::forward_iterator_tag, int32_t>;

using NodeNumRange = boost::iterator_range<NodeNumIter>;
using SeqNumRange = boost::iterator_range<SeqNumIter>;

inline NodeNumRange nodenums(Graph const & g) { return {NodeNum{0}, NodeNum{g.num_nodes()}}; }
inline SeqNumRange seqnums(Graph const & g) { return {SeqNum{0}, SeqNum{g.num_sequences()}}; }

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

inline PosNum end(Sequence const & seq) { return {uint_fast8_t(seq.positions.size())}; }

inline PosNum last_pos(Sequence const & s)
{
	return {PosNum::underlying_type(s.positions.size() - 1)};
}

inline PositionInSequence first_pos_in(SeqNum const s)
{
	return {s, 0};
}

inline PositionInSequence last_pos_in(SeqNum const s, Graph const & g)
{
	return {s, last_pos(g[s])};
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

inline Reoriented<NodeNum> const & from(Graph const & g, Reversible<SeqNum> const s)
{
	return s.reverse ? g.to(*s) : g.from(*s);
}

inline Reoriented<NodeNum> const & to(Graph const & g, Reversible<SeqNum> const s)
{
	return s.reverse ? g.from(*s) : g.to(*s);
}

inline Reoriented<NodeNum> from(Reoriented<SeqNum> const & s, Graph const & g)
{
	ReorientedNode const & n = g.from(*s);
	return {*n, compose(n.reorientation, s.reorientation)};
}

inline Reoriented<NodeNum> to(Reoriented<SeqNum> const & s, Graph const & g)
{
	Reoriented<NodeNum> const & n = g.to(*s);
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

// in/out

vector<Reoriented<Reversible<SeqNum>>>
	in_sequences(Reoriented<NodeNum> const &, Graph const &),
	out_sequences(Reoriented<NodeNum> const &, Graph const &);

vector<Reoriented<Reversible<SegmentInSequence>>>
	in_segments(ReorientedNode const &, Graph const &),
	out_segments(ReorientedNode const &, Graph const &);

vector<Path> in_paths(Graph const &, NodeNum, unsigned size);
vector<Path> out_paths(Graph const &, NodeNum, unsigned size);

// comparison

inline bool operator==(Step const a, Step const b)
{
	return *a == *b && a.reverse == b.reverse;
}

inline bool operator<(Step const a, Step const b)
{
	return std::tie(*a, a.reverse) < std::tie(*b, b.reverse);
}

// misc

vector<ReorientedSegment> neighbours(ReorientedSegment const &, Graph const &, bool open);

inline optional<ReorientedNode> node(Graph const & g, PositionInSequence const pis)
{
	if (pis.position.index == 0) return g.from(pis.sequence);
	if (!next(pis, g)) return g.to(pis.sequence);
	return none;
}

inline void replace(Graph & graph, PositionInSequence const pis, PlayerJoint const j, V3 const v, bool const local)
{
	Position p = graph[pis];
	p[j] = v;
	graph.replace(pis, p, local);
}

optional<Step> step_by_desc(Graph const &, string const & desc, optional<NodeNum> from = none);
optional<NodeNum> node_by_desc(Graph const &, string const & desc);
optional<PositionInSequence> posinseq_by_desc(Graph const & g, string const & desc);

optional<PositionInSequence> node_as_posinseq(Graph const &, NodeNum);
	// may return either the beginning of a sequence or the end

pair<vector<Position>, ReorientedNode> follow(Graph const &, ReorientedNode const &, SeqNum, unsigned frames_per_pos);
ReorientedNode follow(Graph const &, ReorientedNode const &, SeqNum);
NodeNum follow(Graph const &, NodeNum, SeqNum);

bool connected(Graph const &, NodeNum, NodeNum);

inline optional<NodeNum> node_at(Graph const & g, PositionInSequence const pis)
{
	if (pis == first_pos_in(pis.sequence)) return *g.from(pis.sequence);
	if (pis == last_pos_in(pis.sequence, g)) return *g.to(pis.sequence);
	return boost::none;
}

set<string> tags(Graph const &);
set<string> tags_in_desc(vector<string> const &);
set<string> properties_in_desc(vector<string> const &);

inline set<string> tags(Graph::Node const & n)
{
	return tags_in_desc(n.description);
}

inline set<string> tags(Sequence const & s)
{
	return tags_in_desc(s.description);
}

inline set<string> properties(Graph const & g, SeqNum const s)
{
	return properties_in_desc(g[s].description);
}

inline bool is_bidirectional(Sequence const & s)
{
	return properties_in_desc(s.description).count("bidirectional") != 0;
}

inline bool is_top_move(Sequence const & s)
{
	return properties_in_desc(s.description).count("top") != 0;
}

inline bool is_bottom_move(Sequence const & s)
{
	return properties_in_desc(s.description).count("bottom") != 0;
}

inline bool is_tagged(Graph const & g, string const & tag, NodeNum const n)
{
	return tags(g[n]).count(tag) != 0;
}

inline bool is_tagged(Graph const & g, string const & tag, SeqNum const sn)
{
	return tags(g[sn]).count(tag) != 0
		|| (is_tagged(g, tag, *g.from(sn))
		&& is_tagged(g, tag, *g.to(sn)));
}

inline auto tagged_nodes(Graph const & g, string const & tag)
{
	return nodenums(g) | boost::adaptors::filtered(
		[&](NodeNum n){ return is_tagged(g, tag, n); });
}

inline auto tagged_sequences(Graph const & g, string const & tag)
{
	return seqnums(g) | boost::adaptors::filtered(
		[&](SeqNum n){ return is_tagged(g, tag, n); });
}

using TagQuery = set<pair<string /* tag */, bool /* include/exclude */>>;

inline auto match(Graph const & g, TagQuery const & q)
{
	return nodenums(g) | boost::adaptors::filtered(
		[&](NodeNum n){
			foreach (e : q)
			{
				if (is_tagged(g, e.first, n) != e.second)
					return false;
			}
			return true;
			});
}

TagQuery query_for(Graph const &, NodeNum);

set<NodeNum> nodes_around(Graph const &, set<NodeNum> const &, unsigned depth = 1);

set<NodeNum> grow(Graph const &, set<NodeNum>, unsigned depth);

optional<SeqNum> seq_by_arg(Graph const &, string const & arg);
optional<NodeNum> node_by_arg(Graph const &, string const & arg);

inline bool is_sweep(Graph const & g, SeqNum const s)
{
	return g.from(s).reorientation.swap_players != g.to(s).reorientation.swap_players;
}

inline Position at(Location const & l, Graph const & g)
{
	return between(g[from(l.segment)], g[to(l.segment)], l.howFar);
}

inline Position at(Reoriented<Location> const & l, Graph const & g)
{
	return l.reorientation(at(*l, g));
}

inline Position at(Reoriented<PositionInSequence> const & s, Graph const & g)
{
	return s.reorientation(g[*s]);
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

}

#endif
