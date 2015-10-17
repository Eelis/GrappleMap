#ifndef JIUJITSUMAPPER_GRAPH_HPP
#define JIUJITSUMAPPER_GRAPH_HPP

#include "positions.hpp"

using NodeNum = uint16_t;

struct ReorientedNode
{
	NodeNum node;
	Reorientation reorientation;
};

class Graph
{
	struct Edge
	{
		ReorientedNode from, to;
		Sequence sequence;
			// invariant: g[from] == sequence.positions.front()
			// invariant: g[to] == sequences.positions.back()
	};

	std::vector<Position> nodes;
	std::vector<Edge> edges; // indexed by seqnum

	boost::optional<ReorientedNode> is_reoriented_node(Position const & p) const
	{
		for (NodeNum n = 0; n != nodes.size(); ++n)
			if (auto r = is_reoriented(nodes[n], p))
				return ReorientedNode{n, *r};

		return boost::none;
	}

	ReorientedNode find_or_add(Position const & p)
	{
		if (auto m = is_reoriented_node(p))
			return *m;

		nodes.push_back(p);
		return {NodeNum(nodes.size() - 1), noReorientation()};
	}

	void changed(PositionInSequence);

public:

	// construction

	explicit Graph(std::vector<Sequence> const &);

	// const access

	Position const & operator[](PositionInSequence const i) const
	{
		return edges[i.sequence].sequence.positions[i.position];
	}

	Position operator[](ReorientedNode const & n) const
	{
		return apply(n.reorientation, nodes[n.node]);
	}

	Sequence const & sequence(SeqNum const s) const { return edges[s].sequence; }

	ReorientedNode from(SeqNum const s) const { return edges[s].from; }
	ReorientedNode to(SeqNum const s) const { return edges[s].to; }

	unsigned num_sequences() const { return edges.size(); }

	// mutation

	void replace(PositionInSequence, Position const &);
	void clone(PositionInSequence);
	SeqNum new_sequence(Position const &);
	SeqNum insert(Sequence const &);
	boost::optional<SeqNum> erase_sequence(SeqNum); // invalidates seqnums
	boost::optional<PosNum> erase(PositionInSequence); // invalidates posnums
};

inline PosNum last_pos(Graph const & g, SeqNum const s)
{
	return g.sequence(s).positions.size() - 1;
}

inline PositionInSequence first_pos_in(SeqNum const s)
{
	return {s, 0};
}

inline PositionInSequence last_pos_in(Graph const & g, SeqNum const s)
{
	return {s, last_pos(g, s)};
}

inline boost::optional<PositionInSequence> prev(PositionInSequence const pis)
{
	if (pis.position == 0) return boost::none;
	return PositionInSequence{pis.sequence, pis.position - 1};
}

inline boost::optional<PositionInSequence> next(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == last_pos(g, pis.sequence)) return boost::none;
	return PositionInSequence{pis.sequence, pis.position + 1};
}

inline boost::optional<NodeNum> node(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == 0) return g.from(pis.sequence).node;
	if (!next(g, pis)) return g.to(pis.sequence).node;
	return boost::none;
}

inline void replace(Graph & graph, PositionInSequence const pis, PlayerJoint const j, V3 const v)
{
	Position p = graph[pis];
	p[j] = v;
	graph.replace(pis, p);
}

boost::optional<SeqNum> seq_by_desc(Graph const &, std::string const & desc);

#endif
