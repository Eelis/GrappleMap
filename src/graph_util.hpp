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

struct Step { SeqNum seq; bool reverse; };

using Path = vector<Step>;

inline NodeNumRange nodenums(Graph const & g) { return {NodeNum{0}, NodeNum{g.num_nodes()}}; }
inline SeqNumRange seqnums(Graph const & g) { return {SeqNum{0}, SeqNum{g.num_sequences()}}; }

SeqNum insert(Graph &, Sequence const &);
optional<SeqNum> erase_sequence(Graph &, SeqNum);
void split_at(Graph &, PositionInSequence);

// first/last/next/prev

inline PosNum last_pos(Graph const & g, SeqNum const s)
{
	return g[s].positions.size() - 1;
}

inline PositionInSequence first_pos_in(SeqNum const s)
{
	return {s, 0};
}

inline PositionInSequence last_pos_in(Graph const & g, SeqNum const s)
{
	return {s, last_pos(g, s)};
}

inline SegmentInSequence first_segment(SeqNum const s)
{
	return {s, 0};
}

inline SegmentInSequence last_segment(Graph const & g, SeqNum const s)
{
	return {s, unsigned(g[s].positions.size() - 2)};
		// e.g. 3 positions means 2 segments, so last segment index is 1
}

inline ReorientedSegment first_segment(ReorientedSequence const & s)
{
	return {first_segment(s.sequence), s.reorientation};
}

inline ReorientedSegment last_segment(Graph const & g, ReorientedSequence const & s)
{
	return {last_segment(g, s.sequence), s.reorientation};
}

inline optional<PositionInSequence> prev(PositionInSequence const pis)
{
	if (pis.position == 0) return none;
	return PositionInSequence{pis.sequence, pis.position - 1};
}

inline optional<PositionInSequence> next(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == last_pos(g, pis.sequence)) return none;
	return PositionInSequence{pis.sequence, pis.position + 1};
}

inline SegmentInSequence prev(SegmentInSequence const s)
{
	assert(s.segment != 0);
	return {s.sequence, s.segment - 1};
}

inline SegmentInSequence next(SegmentInSequence const s)
{
	return {s.sequence, s.segment + 1};
}

// in/out

vector<SeqNum> in_sequences(Graph const &, NodeNum);
vector<SeqNum> out_sequences(Graph const &, NodeNum);

vector<ReorientedSegment> in_segments(Graph const &, ReorientedNode const &);
vector<ReorientedSegment> out_segments(Graph const &, ReorientedNode const &);

vector<ReorientedSequence> in_sequences(Graph const &, ReorientedNode const &);
vector<ReorientedSequence> out_sequences(Graph const &, ReorientedNode const &);

vector<Step> out_steps(Graph const &, NodeNum);
vector<Step> in_steps(Graph const &, NodeNum);

vector<Path> in_paths(Graph const &, NodeNum, unsigned size);
vector<Path> out_paths(Graph const &, NodeNum, unsigned size);

vector<std::pair<
	vector<Step>, // sequences that end at the node
	vector<Step>>> // sequences that start at the node
		in_out(Graph const &);

// comparison

inline bool operator==(Step const a, Step const b)
{
	return a.seq == b.seq && a.reverse == b.reverse;
}

inline bool operator<(Step const a, Step const b)
{
	return std::tie(a.seq, a.reverse) < std::tie(b.seq, b.reverse);
}

// from/to

inline ReorientedNode const & from(Graph const & g, Step const s)
{
	return s.reverse ? g.to(s.seq) : g.from(s.seq);
}

inline ReorientedNode const & to(Graph const & g, Step const s)
{
	return s.reverse ? g.from(s.seq) : g.to(s.seq);
}

inline ReorientedNode from(ReorientedSequence const & s, Graph const & g)
{
	ReorientedNode const & n = g.from(s.sequence);
	return {n.node, compose(n.reorientation, s.reorientation)};
}

inline ReorientedNode to(ReorientedSequence const & s, Graph const & g)
{
	ReorientedNode const & n = g.to(s.sequence);
	return {n.node, compose(n.reorientation, s.reorientation)};
}

// misc

vector<ReorientedSegment> neighbours(ReorientedSegment const &, Graph const &, bool open);

inline map<NodeNum, std::pair<
	vector<SeqNum>, // sequences that end at the node
	vector<SeqNum>>> // sequences that start at the node
		nodes(Graph const &);

inline optional<ReorientedNode> node(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == 0) return g.from(pis.sequence);
	if (!next(g, pis)) return g.to(pis.sequence);
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
	if (pis == first_pos_in(pis.sequence)) return g.from(pis.sequence).node;
	if (pis == last_pos_in(g, pis.sequence)) return g.to(pis.sequence).node;
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

inline bool is_detailed(Sequence const & s)
{
	return properties_in_desc(s.description).count("detailed") != 0;
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
		|| (is_tagged(g, tag, g.from(sn).node)
		&& is_tagged(g, tag, g.to(sn).node));
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

inline Position at(ReorientedLocation const & l, Graph const & g)
{
	return l.reorientation(at(l.location, g));
}

inline optional<PositionInSequence> position(Location const & l)
{
	if (l.howFar == 0) return from(l.segment);
	if (l.howFar == 1) return to(l.segment);
	return boost::none;
}

}

#endif
