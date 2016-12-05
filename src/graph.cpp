#include "graph_util.hpp"

namespace GrappleMap {

void Graph::changed(PositionInSequence const pis)
{
	Edge & edge = edges.at(pis.sequence.index);

	if (pis.position.index == 0)
	{
		Reoriented<NodeNum> const new_from = find_or_add(edge.sequence.positions.front());
		NodeNum const old_from = *edge.from;

		edge.from = new_from;

		if (*new_from != old_from)
		{
			std::cerr << "Front of sequence is now a different node." << std::endl;

			compute_in_out(old_from);
			compute_in_out(*new_from);
		}
	}
	else if (!next(pis, *this))
	{
		Reoriented<NodeNum> const new_to = find_or_add(edge.sequence.positions.back());
		NodeNum const old_to = *edge.to;

		edge.to = new_to;

		if (*new_to != old_to)
		{
			std::cerr << "Back of sequence is now a different node." << std::endl;

			compute_in_out(old_to);
			compute_in_out(*new_to);
		}
	}
}

void Graph::replace(PositionInSequence const pis, Position const & p, bool const local)
{
	edges.at(pis.sequence.index).sequence.positions.at(pis.position.index) = p;

	optional<ReorientedNode> const rn = node(*this, pis);
	if (!local && rn)
	{
		nodes[(*rn)->index].position = inverse(rn->reorientation)(p);
		assert(basicallySame((*this)[*rn], p));

		foreach (e : edges)
		{
			if (*e.from == **rn)
				e.sequence.positions.front() = (*this)[e.from];

			if (*e.to == **rn)
				e.sequence.positions.back() = (*this)[e.to];
		}
	}
	else changed(pis);

	std::cerr << "Replaced position " << pis << std::endl;
}

void Graph::split_segment(Location const loc)
{
	auto & positions =  edges.at(loc.segment.sequence.index).sequence.positions;
	Position const p = at(loc, *this);
	positions.insert(positions.begin() + loc.segment.segment.index + 1, p);
}

void Graph::clone(PositionInSequence const pis) // todo: remove
{
	auto & positions =  edges.at(pis.sequence.index).sequence.positions;
	Position const p = positions.at(pis.position.index);
	positions.insert(positions.begin() + pis.position.index, p);
}

void Graph::insert_sequences(vector<Sequence> && v) // for bulk, more efficient than individually
{
	foreach (s : v)
		edges.push_back(Edge{
			find_or_add(s.positions.front()),
			find_or_add(s.positions.back()),
			std::move(s)});

	foreach (n : nodenums(*this))
		compute_in_out(n);
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

		compute_in_out(*e.from);
		compute_in_out(*e.to);
	}
	else if (num)
	{
		edges.erase(edges.begin() + num->index);

		foreach (n : nodenums(*this))
			compute_in_out(n);
	}
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

	positions.erase(positions.begin() + pis.position.index);

	auto const pos = std::min(pis.position, last_pos((*this)[pis.sequence]));

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

ReorientedNode Graph::find_or_add(Position const & p)
{
	if (auto m = is_reoriented_node(p))
		return *m;

	nodes.push_back(Node{p, vector<string>(), {}, {}, {}});

	NodeNum const nn{uint16_t(nodes.size() - 1)};

	compute_in_out(nn);

	return ReorientedNode{nn, PositionReorientation{}};
}

void Graph::compute_in_out(NodeNum const n)
{
	Node & node = nodes[n.index];

	node.in.clear();
	node.out.clear();

	for (SeqNum s{0}; s.index != edges.size(); ++s.index)
	{
		ReorientedNode const
			& from_ = from(s),
			& to_ = to(s);

		if (*to_ == n) node.in.push_back({s, false});
		if (*from_ == n) node.out.push_back({s, false});

		if (is_bidirectional(edges[s.index].sequence))
		{
			if (*from_ == n) node.in.push_back({s, true});
			if (*to_ == n) node.out.push_back({s, true});
		}
	}
}

}
