#ifndef JIUJITSUMAPPER_GRAPH_HPP
#define JIUJITSUMAPPER_GRAPH_HPP

#include "positions.hpp"
#include <map>
#include <iostream>

struct NodeNum { uint16_t index; };

inline bool operator==(NodeNum const a, NodeNum const b) { return a.index == b.index; }
inline bool operator!=(NodeNum const a, NodeNum const b) { return a.index != b.index; }
inline bool operator<(NodeNum const a, NodeNum const b) { return a.index < b.index; }

inline std::ostream & operator<<(std::ostream & o, NodeNum const n)
{
	return o << "node" << n.index;
}

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

struct Graph
{
	struct Node
	{
		Position position;
		vector<string> description;
	};

	struct Edge
	{
		ReorientedNode from, to;
		Sequence sequence;
			// invariant: g[from] == sequence.positions.front()
			// invariant: g[to] == sequences.positions.back()
	};

private:

	vector<Node> nodes;
	vector<Edge> edges; // indexed by seqnum

	optional<ReorientedNode> is_reoriented_node(Position const & p) const
	{
		for (NodeNum n{0}; n.index != nodes.size(); ++n.index)
			if (auto r = is_reoriented(nodes[n.index].position, p))
				return ReorientedNode{n, *r};

		return none;
	}

	ReorientedNode find_or_add(Position const & p)
	{
		if (auto m = is_reoriented_node(p))
			return *m;

		nodes.push_back({p, {}});
		return ReorientedNode{NodeNum{uint16_t(nodes.size() - 1)}, PositionReorientation{}};
	}

	void changed(PositionInSequence);

public:

	// construction

	explicit Graph(std::vector<Node> const &, std::vector<Sequence> const &);

	// const access

	Position const & operator[](PositionInSequence const i) const
	{
		return edges[i.sequence.index].sequence.positions[i.position];
	}

	Position operator[](ReorientedNode const & n) const
	{
		return n.reorientation(nodes[n.node.index].position);
	}

	Node const & operator[](NodeNum const n) const { return nodes[n.index]; }

	Sequence const & operator[](SeqNum const s) const { return edges[s.index].sequence; }

	ReorientedNode from(SeqNum const s) const { return edges[s.index].from; }
	ReorientedNode to(SeqNum const s) const { return edges[s.index].to; }

	uint16_t num_sequences() const { return edges.size(); }
	uint16_t num_nodes() const { return nodes.size(); }

	// mutation

	void insert(Node n) { nodes.emplace_back(move(n)); }
	void replace(PositionInSequence, Position const &, bool local);
		// The local flag only affects the case where the position denotes a node.
		// In that case, if local is true, the existing node and connecting sequences
		// will not be updated, but the sequence will detach from the node instead,
		// and either end up on a new node, or connect to another existing node.
	void clone(PositionInSequence);
	optional<PosNum> erase(PositionInSequence); // invalidates posnums

	void set(optional<SeqNum>, optional<Sequence>);
		// no seqnum means insert
		// no sequence means erase
		// neither means noop
		// both means replace
};

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
	std::vector<SeqNum>, // sequences that end at the node
	std::vector<SeqNum>>> // sequences that start at the node
		nodes(Graph const & g)
{
	std::map<NodeNum, std::pair<std::vector<SeqNum>, std::vector<SeqNum>>> m;

	for (SeqNum sn{0}; sn.index != g.num_sequences(); ++sn.index)
	{
		m[g.to(sn).node].first.push_back(sn);
		m[g.from(sn).node].second.push_back(sn);
	}

	return m;
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

optional<SeqNum> seq_by_desc(Graph const &, std::string const & desc);

optional<PositionInSequence> node_as_posinseq(Graph const &, NodeNum);
	// may return either the beginning of a sequence or the end

#endif
