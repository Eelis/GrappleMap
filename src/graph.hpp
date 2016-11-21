#ifndef GRAPPLEMAP_GRAPH_HPP
#define GRAPPLEMAP_GRAPH_HPP

#include "positions.hpp"

namespace GrappleMap {

struct NodeNum { uint16_t index; };

inline NodeNum & operator++(NodeNum & n) { ++n.index; return n; }
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
		optional<unsigned> line_nr;
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

	optional<ReorientedNode> is_reoriented_node(Position const &) const;

	ReorientedNode find_or_add(Position const & p)
	{
		if (auto m = is_reoriented_node(p))
			return *m;

		nodes.push_back(Node{p, vector<string>(), {}});
		return ReorientedNode{NodeNum{uint16_t(nodes.size() - 1)}, PositionReorientation{}};
	}

	void changed(PositionInSequence);

public:

	// construction

	explicit Graph(vector<Node> const &, vector<Sequence> const &);

	// const access

	Position const & operator[](PositionInSequence const i) const
	{
		return (*this)[i.sequence].positions[i.position];
	}

	Position operator[](ReorientedNode const & n) const
	{
		return n.reorientation(nodes[n.node.index].position);
	}

	Node const & operator[](NodeNum const n) const { return nodes[n.index]; }

	Sequence const & operator[](SeqNum const s) const { return edges[s.index].sequence; }

	ReorientedNode const & from(SeqNum const s) const { return edges[s.index].from; }
	ReorientedNode const & to(SeqNum const s) const { return edges[s.index].to; }

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

}

#endif
