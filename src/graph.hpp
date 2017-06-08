#ifndef GRAPPLEMAP_GRAPH_HPP
#define GRAPPLEMAP_GRAPH_HPP

#include "reoriented.hpp"

namespace GrappleMap {

struct Graph
{
	struct Node
	{
		Position position;
		vector<string> description;
		optional<unsigned> line_nr;
		vector<Reversible<SeqNum>> in, out;
			// only bidirectional transitions appear as reversed seqs here
		vector<Reversible<SeqNum>> in_out;
			// reverse means ends in node, non-reverse means starts in node
			// transitions only appear in their primary direction here
	};

	struct Edge: Sequence
	{
		ReorientedNode from, to;
			// invariant: g[from] == sequence.positions.front()
			// invariant: g[to] == sequences.positions.back()

		Edge(ReorientedNode f, ReorientedNode t, Sequence s)
			: Sequence(std::move(s)), from(f), to(t)
		{}
	};

private:

	vector<Node> nodes;
	vector<Edge> edges;

	optional<ReorientedNode> is_reoriented_node(Position const &) const;

	ReorientedNode find_or_add(Position const &);

	void changed(PositionInSequence);
	void compute_in_out(NodeNum);
	ReorientedNode * node(PositionInSequence);

public:

	Graph() {}

	Graph & operator=(Graph &&) = default;
	Graph(Graph &&) = default;

	Graph & operator=(Graph const &) = default;
	Graph(Graph const &) = default;

	// const access

	Position operator[](ReorientedNode const & n) const
	{
		return n.reorientation(nodes[n->index].position);
	}

	Node const & operator[](NodeNum const n) const { return nodes[n.index]; }
	Edge const & operator[](SeqNum const s) const { return edges[s.index]; }

	uint16_t num_sequences() const { return edges.size(); }
	uint16_t num_nodes() const { return nodes.size(); }

	// mutation

	void insert_node(Position pos, vector<string> desc, optional<unsigned> line)
	{
		nodes.emplace_back(Node{pos, desc, line, {}, {}, {}});
		compute_in_out(NodeNum{NodeNum::underlying_type(nodes.size() - 1)});
	}

	void insert_sequences(vector<Sequence> &&); // for bulk, more efficient than individually with set()

	void replace(PositionInSequence, Position, bool local);
		// The local flag only affects the case where the position denotes a node.
		// In that case, if local is true, the existing node and connecting sequences
		// will not be updated, but the sequence will detach from the node instead,
		// and either end up on a new node, or connect to another existing node.
	void clone(PositionInSequence);
	optional<PosNum> erase(PositionInSequence); // invalidates posnums

	void split_segment(Location);

	void set(optional<SeqNum>, optional<Sequence>);
		// no seqnum means insert
		// no sequence means erase
		// neither means noop
		// both means replace
};

}

#endif
