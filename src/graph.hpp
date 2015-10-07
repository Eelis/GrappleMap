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
			// invariant: get(g, from) == sequence.positions.front()
			// invariant: get(g, to) == sequences.positions.back()
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

Graph::Graph(std::vector<Sequence> const & sequences)
{
	foreach (s : sequences) insert(s);
	std::cerr << "Loaded " << nodes.size() << " nodes and " << edges.size() << " edges." << std::endl;
}

PosNum last_pos(Graph const & g, SeqNum const s)
{
	return g.sequence(s).positions.size() - 1;
}

boost::optional<PositionInSequence> prev(PositionInSequence const pis)
{
	if (pis.position == 0) return boost::none;
	return PositionInSequence{pis.sequence, pis.position - 1};
}

boost::optional<PositionInSequence> next(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == last_pos(g, pis.sequence)) return boost::none;
	return PositionInSequence{pis.sequence, pis.position + 1};
}

void Graph::changed(PositionInSequence const pis)
{
	Edge & edge = edges.at(pis.sequence);

	if (pis.position == 0)
	{
		auto const new_from = find_or_add(edge.sequence.positions.front());

		if (new_from.node != edge.from.node)
			std::cerr << "Front of sequence is now a different node." << std::endl;

		edge.from = new_from;
	}
	else if (!next(*this, pis))
	{
		auto const new_to = find_or_add(edge.sequence.positions.back());

		if (new_to.node != edge.to.node)
			std::cerr << "Back of sequence is now a different node." << std::endl;

		edge.to = new_to;
	}
}

boost::optional<NodeNum> node(Graph const & g, PositionInSequence const pis)
{
	if (pis.position == 0) return g.from(pis.sequence).node;
	if (!next(g, pis)) return g.to(pis.sequence).node;
	return boost::none;
}

void Graph::replace(PositionInSequence const pis, Position const & p)
{
	edges.at(pis.sequence).sequence.positions.at(pis.position) = p;
	changed(pis);

	std::cerr << "Replaced position " << pis << std::endl;
}

void replace(Graph & graph, PositionInSequence const pis, PlayerJoint const j, V3 const v)
{
	Position p = graph[pis];
	p[j] = v;
	graph.replace(pis, p);
}

void Graph::clone(PositionInSequence const pis)
{
	auto & positions =  edges.at(pis.sequence).sequence.positions;
	Position const p = positions.at(pis.position);
	positions.insert(positions.begin() + pis.position, p);
}

SeqNum Graph::insert(Sequence const & sequence)
{
	auto const num = edges.size();

	std::cerr <<
		"Inserted sequence " << num <<
		" (\"" << sequence.description << "\")"
		" of size " << sequence.positions.size() << std::endl;

	edges.push_back({
		find_or_add(sequence.positions.front()),
		find_or_add(sequence.positions.back()),
		sequence});

	return num;
}

boost::optional<SeqNum> Graph::erase_sequence(SeqNum const sn)
{
	if (edges.size() == 1)
	{
		std::cerr << "Cannot erase last sequence." << std::endl;
		return boost::none;
	}

	auto const & seq = sequence(sn);

	std::cerr <<
		"Erasing sequence " << sn <<
		" (\"" << seq.description << "\")"
		" and the " << seq.positions.size() << " positions in it." << std::endl;

	edges.erase(edges.begin() + sn);

	return sn == 0 ? 0 : sn - 1;
}

boost::optional<PosNum> Graph::erase(PositionInSequence const pis)
{
	auto & edge = edges.at(pis.sequence);
	auto & positions = edge.sequence.positions;

	if (positions.size() == 2)
	{
		std::cerr << "Cannot erase either of last two elements in sequence." << std::endl;
		return boost::none;
	}

	positions.erase(positions.begin() + pis.position);

	auto const pos = std::min(pis.position, last_pos(*this, pis.sequence));

	changed({pis.sequence, pos});

	return pos;
}

#endif
