#ifndef JIUJITSUMAPPER_GRAPH_UTIL_HPP
#define JIUJITSUMAPPER_GRAPH_UTIL_HPP

#include "graph.hpp"
#include <map>
#include <boost/range/counting_range.hpp>
#include <boost/range/adaptor/filtered.hpp>

using NodeNumIter = boost::counting_iterator<NodeNum, boost::incrementable_traversal_tag, uint32_t>;
using SeqNumIter = boost::counting_iterator<SeqNum, boost::incrementable_traversal_tag, uint32_t>;

using NodeNumRange = boost::iterator_range<NodeNumIter>;
using SeqNumRange = boost::iterator_range<SeqNumIter>;

inline NodeNumRange nodenums(Graph const & g) { return {NodeNum{0}, NodeNum{g.num_nodes()}}; }
inline SeqNumRange seqnums(Graph const & g) { return {SeqNum{0}, SeqNum{g.num_sequences()}}; }

SeqNum insert(Graph &, Sequence const &);
optional<SeqNum> erase_sequence(Graph &, SeqNum);
void split_at(Graph &, PositionInSequence);

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

inline std::map<NodeNum, std::pair<
	vector<SeqNum>, // sequences that end at the node
	vector<SeqNum>>> // sequences that start at the node
		nodes(Graph const & g)
{
	std::map<NodeNum, std::pair<vector<SeqNum>, vector<SeqNum>>> m;

	foreach(sn : seqnums(g))
	{
		m[g.to(sn).node].first.push_back(sn);
		m[g.from(sn).node].second.push_back(sn);
	}

	return m;
}

inline vector<SeqNum> in(Graph const & g, NodeNum const n)
{
	vector<SeqNum> v;

	foreach(sn : seqnums(g))
		if (g.to(sn).node == n)
			v.push_back(sn);

	return v;
}

inline vector<SeqNum> out(Graph const & g, NodeNum const n)
{
	vector<SeqNum> v;

	foreach(sn : seqnums(g))
		if (g.from(sn).node == n)
			v.push_back(sn);

	return v;
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

optional<SeqNum> seq_by_desc(Graph const &, string const & desc);
optional<NodeNum> node_by_desc(Graph const &, string const & desc);

optional<PositionInSequence> node_as_posinseq(Graph const &, NodeNum);
	// may return either the beginning of a sequence or the end

pair<vector<Position>, ReorientedNode> follow(Graph const &, ReorientedNode const &, SeqNum, unsigned const frames_per_pos);

bool connected(Graph const &, NodeNum, NodeNum);

inline optional<NodeNum> node_at(Graph const & g, PositionInSequence const pis)
{
	if (pis == first_pos_in(pis.sequence)) return g.from(pis.sequence).node;
	if (pis == last_pos_in(g, pis.sequence)) return g.to(pis.sequence).node;
	return boost::none;
}

set<string> tags(Graph const &);
set<string> tags(Graph const &, NodeNum const &);

inline auto tagged_nodes(Graph const & g, string const & tag)
{
	using boost::adaptors::filtered;
	return nodenums(g) | filtered(
		[&](NodeNum const & n){ return tags(g, n).count(tag) != 0; });
}

set<NodeNum> nodes_around(Graph const &, set<NodeNum> const &, unsigned depth = 1);

set<NodeNum> grow(Graph const &, set<NodeNum>, unsigned depth);

inline bool is_internal(Graph const & g, set<NodeNum> const & nodes, SeqNum const seq)
{
	return nodes.count(g.from(seq).node) && nodes.count(g.to(seq).node);
}

optional<SeqNum> seq_by_arg(Graph const &, string const & arg);
optional<NodeNum> node_by_arg(Graph const &, string const & arg);

#endif
