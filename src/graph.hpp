#ifndef GRAPPLEMAP_GRAPH_HPP
#define GRAPPLEMAP_GRAPH_HPP

#include "reoriented.hpp"
#include "rewindable.hpp"

namespace GrappleMap {

struct NamedPosition
{
	Position position;
	vector<string> description;
	optional<unsigned> line_nr;
};

inline Position & follow(Sequence & s, PosNum p) // todo: move
{
	return s.positions[p.index];
}

struct Graph
{
	enum class NodeModifyPolicy { propagate, local, unintended };

	struct Node: NamedPosition
	{
		vector<Reversible<SeqNum>> in, out;
			// only bidirectional transitions appear as reversed seqs here
		vector<Reversible<SeqNum>> in_out;
			// reverse means ends in node, non-reverse means starts in node
			// transitions only appear in their primary direction here

		bool dirty = false;

		explicit Node(NamedPosition p): NamedPosition(move(p)) {}
	};

	struct Edge: Sequence
	{
		ReorientedNode from, to;
			// invariant: g[from] == sequence.positions.front()
			// invariant: g[to] == sequences.positions.back()

		bool dirty = false;

		Edge(ReorientedNode f, ReorientedNode t, Sequence s)
			: Sequence(std::move(s)), from(f), to(t)
		{}

		friend Position & follow(Edge & s, PosNum p)
			// todo: should not be necessary since we have the one for Sequence...
		{
			return s.positions[p.index];
		}
	};

private:

	struct Data
	{
		vector<Node> nodes;
		vector<Edge> edges;

		friend Node & follow(Data & d, NodeNum n) { return d.nodes[n.index]; }
		friend Edge & follow(Data & d, SeqNum s) { return d.edges[s.index]; }
	};

	Rewindable<Data> data;

	optional<ReorientedNode> is_reoriented_node(Position const &) const;

	ReorientedNode find_or_add(Position const &);

	void changed(PositionInSequence);
	void compute_in_out(NodeNum);

	optional<Rewindable<Data>::OnPath<SeqNum, Reoriented<NodeNum> Edge::*>>
		node(PositionInSequence);

	void mark_dirty(SeqNum);

public:

	Graph(vector<NamedPosition>, vector<Sequence>);

	Graph & operator=(Graph &&) = default;
	Graph(Graph &&) = default;

	Graph & operator=(Graph const &) = default;
	Graph(Graph const &) = default;

	// const access

	Position operator[](ReorientedNode const & n) const
	{
		return n.reorientation(data->nodes[n->index].position);
	}

	Node const & operator[](NodeNum const n) const { return data->nodes[n.index]; }
	Edge const & operator[](SeqNum const s) const { return data->edges[s.index]; }

	uint16_t num_sequences() const { return data->edges.size(); }
	uint16_t num_nodes() const { return data->nodes.size(); }

	// mutation

	void replace(PositionInSequence, Position, NodeModifyPolicy);
	optional<PosNum> erase(PositionInSequence); // invalidates posnums

	void split_segment(Location);

	void set(optional<SeqNum>, optional<Sequence>);
		// no seqnum means insert
		// no sequence means erase
		// neither means noop
		// both means replace

	void rewind_point() { data.rewind_point(); }
	void rewind() { data.rewind(); }

	void set_description(NodeNum, string const &);
	void set_description(SeqNum, string const &);
};

}

#endif
