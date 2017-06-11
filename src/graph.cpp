#include "graph_util.hpp"

namespace GrappleMap {

void Graph::changed(PositionInSequence const pis)
{
	Edge & edge = edges.at(pis.sequence.index);
	edge.dirty = true;

	if (ReorientedNode * const rn = node(pis))
	{
		Reoriented<NodeNum> const newn = find_or_add(edge.positions[pis.position.index]);

		NodeNum const oldn = **rn;

		*rn = newn;

		if (*newn != oldn)
		{
			std::cerr << "End of sequence is now a different node." << std::endl;

			compute_in_out(oldn);
			compute_in_out(*newn);
		}
	}
}

ReorientedNode * Graph::node(PositionInSequence const pis)
{
	if (pis.position.index == 0)
		return &edges[pis.sequence.index].from;
	if (!next(pis, *this))
		return &edges[pis.sequence.index].to;
	return nullptr;
}

void Graph::replace(PositionInSequence const pis, Position p, NodeModifyPolicy const policy)
{
	apply_limits(p);

	Edge & edge = edges.at(pis.sequence.index);
	Position & stored = edge.positions.at(pis.position.index);

	auto apply = [&]{ stored = p; edge.dirty = true; };

	if (stored == p) return;

	if (ReorientedNode * const rn = node(pis))
	{
		if (auto reo = is_reoriented(nodes[(*rn)->index].position, p))
		{
//			std::cerr << "Recognized position as mere reorientation.\n";
			apply();
			rn->reorientation = *reo;
			assert(basicallySame((*this)[*rn], p));
		}
		else switch (policy)
		{
			case NodeModifyPolicy::propagate:
			{
	//			std::cerr << "Change to connecting position, updating connected edges.\n";
				apply();
				Node & node = nodes[(*rn)->index];
				node.position = inverse(rn->reorientation)(p);
				node.dirty = true;
				assert(basicallySame((*this)[*rn], p));

				foreach (e : edges)
				{
					if (*e.from == **rn)
					{
						e.positions.front() = (*this)[e.from];
						e.dirty = true;
					}

					if (*e.to == **rn)
					{
						e.positions.back() = (*this)[e.to];
						e.dirty = true;
					}
				}
				break;
			}
			case NodeModifyPolicy::local:
			{
				apply();

				Reoriented<NodeNum> const newn = find_or_add(p);

				NodeNum const oldn = **rn;

				*rn = newn;

				if (*newn != oldn)
				{
					std::cerr << "End of sequence is now a different node." << std::endl;

					compute_in_out(oldn);
					compute_in_out(*newn);
				}

				break;
			}
			case NodeModifyPolicy::unintended:
			{
				assert(!"accidental attempt at non-reorienting edit of connecting position");
				break;
			}
		}
	}
	else apply();
}

void Graph::split_segment(Location const loc)
{
	Edge & edge = edges.at(loc.segment.sequence.index);
	edge.dirty = true;
	auto & positions = edge.positions;
	Position const p = at(loc, *this);
	positions.insert(positions.begin() + loc.segment.segment.index + 1, p);
}

void Graph::clone(PositionInSequence const pis) // todo: remove
{
	Edge & edge = edges.at(pis.sequence.index);
	edge.dirty = true;
	auto & positions = edge.positions;
	Position const p = positions.at(pis.position.index);
	positions.insert(positions.begin() + pis.position.index, p);
}

void Graph::set(optional<SeqNum> const num, optional<Sequence> seq)
{
	if (seq)
	{
		ReorientedNode const
			from = find_or_add(seq->positions.front()),
			to = find_or_add(seq->positions.back());

		Edge e{from, to, move(*seq)};
		e.dirty = true;

		if (num)
			edges[num->index] = move(e);
		else
			edges.push_back(move(e));

		compute_in_out(*from);
		compute_in_out(*to);
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

	if (edge.positions.size() == 2)
	{
		std::cerr << "Cannot erase either of last two elements in sequence." << std::endl;
		return none;
	}

	edge.positions.erase(edge.positions.begin() + pis.position.index);

	auto const pos = std::min(pis.position, last_pos((*this)[pis.sequence]));

	changed({pis.sequence, pos});

	return pos;
}

optional<Reoriented<NodeNum>> Graph::is_reoriented_node(Position const & p) const
{
	foreach(n : nodenums(*this))
		if (auto r = is_reoriented(nodes[n.index].position, p))
			return n * *r;

	return none;
}

Reoriented<NodeNum> Graph::find_or_add(Position const & p)
{
	if (auto m = is_reoriented_node(p))
		return *m;

	nodes.emplace_back(NamedPosition{p, vector<string>(), {}});
		// no need for dirty flag because these won't be persisted anyway

	NodeNum const nn{uint16_t(nodes.size() - 1)};

	compute_in_out(nn);

	return nn * PositionReorientation{};
}

void Graph::compute_in_out(NodeNum const n)
{
	Node & node = nodes[n.index];

	node.in.clear();
	node.out.clear();
	node.in_out.clear();

	for (SeqNum s{0}; s.index != edges.size(); ++s.index)
	{
		ReorientedNode const
			& from_ = (*this)[s].from,
			& to_ = (*this)[s].to;

		if (*to_ == n)
		{
			node.in.push_back({s, false});
			node.in_out.push_back({s, true});
		}

		if (*from_ == n)
		{
			node.out.push_back({s, false});
			node.in_out.push_back({s, false});
		}

		if (edges[s.index].bidirectional)
		{
			if (*from_ == n) node.in.push_back({s, true});
			if (*to_ == n) node.out.push_back({s, true});
		}
	}
}

Graph::Graph(vector<NamedPosition> pp, vector<Sequence> ss)
{
	foreach(p : pp)
		nodes.emplace_back(move(p));

	foreach (s : ss)
	{
		ReorientedNode const
			from = find_or_add(s.positions.front()),
			to = find_or_add(s.positions.back());
		edges.push_back(Edge{from, to, move(s)});
	}

	foreach (n : nodenums(*this))
		compute_in_out(n);
}

}
