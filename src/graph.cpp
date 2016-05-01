#include "graph_util.hpp"

namespace GrappleMap {

Graph::Graph(vector<Node> const & nodes, vector<Sequence> const & sequences)
{
	foreach (n : nodes) insert(n);
	foreach (s : sequences) GrappleMap::insert(*this, s);
	std::cerr << "Loaded " << nodes.size() << " nodes and " << edges.size() << " edges." << std::endl;
}

void Graph::changed(PositionInSequence const pis)
{
	Edge & edge = edges.at(pis.sequence.index);

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

void Graph::replace(PositionInSequence const pis, Position const & p, bool const local)
{
	edges.at(pis.sequence.index).sequence.positions.at(pis.position) = p;

	optional<ReorientedNode> const rn = node(*this, pis);
	if (!local && rn)
	{
		nodes[rn->node.index].position = inverse(rn->reorientation)(p);
		assert(basicallySame((*this)[*rn], p));

		foreach (e : edges)
		{
			if (e.from.node == rn->node)
				e.sequence.positions.front() = (*this)[e.from];

			if (e.to.node == rn->node)
				e.sequence.positions.back() = (*this)[e.to];
		}
	}
	else changed(pis);

	std::cerr << "Replaced position " << pis << std::endl;
}

void Graph::clone(PositionInSequence const pis)
{
	auto & positions =  edges.at(pis.sequence.index).sequence.positions;
	Position const p = positions.at(pis.position);
	positions.insert(positions.begin() + pis.position, p);
}

void Graph::set(optional<SeqNum> const num, optional<Sequence> const seq)
{
	if (seq)
	{
		Edge e{
			find_or_add(seq->positions.front()),
			find_or_add(seq->positions.back()),
			*seq};

		if (num)
			edges[num->index] = e;
		else
			edges.push_back(e);
	}
	else if (num)
		edges.erase(edges.begin() + num->index);
}

optional<PosNum> Graph::erase(PositionInSequence const pis)
{
	auto & edge = edges.at(pis.sequence.index);
	auto & positions = edge.sequence.positions;

	if (positions.size() == 2)
	{
		std::cerr << "Cannot erase either of last two elements in sequence." << std::endl;
		return none;
	}

	positions.erase(positions.begin() + pis.position);

	auto const pos = std::min(pis.position, last_pos(*this, pis.sequence));

	changed({pis.sequence, pos});

	return pos;
}

optional<ReorientedNode> Graph::is_reoriented_node(Position const & p) const
{
	foreach(n : nodenums(*this))
		if (auto r = is_reoriented(nodes[n.index].position, p))
			return ReorientedNode{n, *r};

	return none;
}

}
