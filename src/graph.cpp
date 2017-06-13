#include "graph_util.hpp"
#include "metadata.hpp"
#include <boost/algorithm/string/split.hpp>

namespace GrappleMap {

void Graph::changed(PositionInSequence const pis)
{
	data[pis.sequence][&Edge::dirty] = true;

	Edge const & edge = data->edges.at(pis.sequence.index);

	if (auto rn = node(pis))
	{
		Reoriented<NodeNum> const newn = find_or_add(edge.positions[pis.position.index]);

		NodeNum const oldn = ***rn;

		*rn = newn;

		if (*newn != oldn)
		{
			std::cerr << "End of sequence is now a different node." << std::endl;

			compute_in_out(oldn);
			compute_in_out(*newn);
		}
	}
}

optional<Rewindable<Graph::Data>::OnPath<SeqNum, Reoriented<NodeNum> Graph::Edge::*>>
	Graph::node(PositionInSequence const pis)
{
	if (pis.position.index == 0) return data[pis.sequence][&Edge::from];
	if (!next(pis, *this)) return data[pis.sequence][&Edge::to];
	return {};
}

void Graph::replace(PositionInSequence const pis, Position p, NodeModifyPolicy const policy)
{
	apply_limits(p);

	if (data->edges.at(pis.sequence.index).positions.at(pis.position.index) == p) return;

	auto apply = [&]{
			auto edge = data[pis.sequence];
			edge[pis.position] = p;
			edge[&Edge::dirty] = true;
		};


	if (auto orn = node(pis))
	{
		auto & rn = *orn;

		switch (policy)
		{
			case NodeModifyPolicy::propagate:
			{
	//			std::cerr << "Change to connecting position, updating connected edges.\n";
	//			apply();

				auto && node = data[**rn];

				node[&Node::position] = inverse(rn->reorientation)(p);
				node[&Node::dirty] = true;

				assert(basicallySame((*this)[*rn], p));

				foreach (s : seqnums(*this))
				{
					auto && we = data[s];

					if (*we->from == **rn)
					{
						we[&Edge::positions][0] = (*this)[we->from];
						we[&Edge::dirty] = true;
					}

					if (*we->to == **rn)
					{
						we[&Edge::positions][we->positions.size() - 1] = (*this)[we->to];
						we[&Edge::dirty] = true;
					}
				}

				break;
			}
			case NodeModifyPolicy::unintended:
			{
				auto reo = is_reoriented(data->nodes[(*rn)->index].position, p);
				assert(reo && "accidental attempt at non-reorienting edit of connecting position");
				apply();
				rn[&ReorientedNode::reorientation] = *reo;
				assert(basicallySame((*this)[rn], p));
				break;
			}
			case NodeModifyPolicy::local:
			{
				apply();

				if (auto reo = is_reoriented(data->nodes[(*rn)->index].position, p))
				{
		//			std::cerr << "Recognized position as mere reorientation.\n";
					rn[&ReorientedNode::reorientation] = *reo;
					assert(basicallySame((*this)[rn], p));
				}
				else
				{
					Reoriented<NodeNum> const newn = find_or_add(p);

					NodeNum const oldn = **rn;

					rn = newn;

					if (*newn != oldn)
					{
						std::cerr << "End of sequence is now a different node." << std::endl;

						compute_in_out(oldn);
						compute_in_out(*newn);
					}

				}
			}
		}
	}
	else apply();
}

void Graph::mark_dirty(SeqNum const seq)
{
	data[seq][&Edge::dirty] = true;
}

void Graph::split_segment(Location const loc)
{
	mark_dirty(loc.segment.sequence);

	data[loc.segment.sequence][&Edge::positions].insert(
		loc.segment.segment.index + 1, at(loc, *this));
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
			data[*num] = move(e);
		else
			data[&Data::edges].push_back(move(e));

		compute_in_out(*from);
		compute_in_out(*to);
	}
	else if (num)
	{
		data[&Data::edges].erase(num->index);

		foreach (n : nodenums(*this))
			compute_in_out(n);
	}
}

optional<PosNum> Graph::erase(PositionInSequence const pis)
{
	auto const & edge = data->edges.at(pis.sequence.index);

	if (edge.positions.size() == 2)
	{
		std::cerr << "Cannot erase either of last two elements in sequence." << std::endl;
		return none;
	}

	data[pis.sequence][&Edge::positions].erase(pis.position.index);

	auto const pos = std::min(pis.position, last_pos((*this)[pis.sequence]));

	changed({pis.sequence, pos});

	return pos;
}

optional<Reoriented<NodeNum>> Graph::is_reoriented_node(Position const & p) const
{
	foreach(n : nodenums(*this))
		if (auto r = is_reoriented(data->nodes[n.index].position, p))
			return n * *r;

	return none;
}

Reoriented<NodeNum> Graph::find_or_add(Position const & p)
{
	if (auto m = is_reoriented_node(p))
		return *m;

	data[&Data::nodes].push_back(
		Node(NamedPosition{p, vector<string>(), {}}));
			// no need for dirty flag because these won't be persisted anyway

	NodeNum const nn{uint16_t(data->nodes.size() - 1)};

	compute_in_out(nn);

	return nn * PositionReorientation{};
}

void Graph::compute_in_out(NodeNum const n)
{
	vector<Reversible<SeqNum>> in, out, in_out;

	foreach (s : seqnums(*this))
	{
		ReorientedNode const
			& from_ = (*this)[s].from,
			& to_ = (*this)[s].to;

		if (*to_ == n)
		{
			in.push_back({s, false});
			in_out.push_back({s, true});
		}

		if (*from_ == n)
		{
			out.push_back({s, false});
			in_out.push_back({s, false});
		}

		if (data->edges[s.index].bidirectional)
		{
			if (*from_ == n) in.push_back({s, true});
			if (*to_ == n) out.push_back({s, true});
		}
	}

	auto && node = data[n];

	if (node[&Node::in    ] != in    ) node[&Node::in    ] = in;
	if (node[&Node::   out] !=    out) node[&Node::   out] =    out;
	if (node[&Node::in_out] != in_out) node[&Node::in_out] = in_out;
}

Graph::Graph(vector<NamedPosition> pp, vector<Sequence> ss)
{
	foreach(p : pp)
		data[&Data::nodes].push_back(Node(move(p)));

	foreach (s : ss)
	{
		ReorientedNode const
			from = find_or_add(s.positions.front()),
			to = find_or_add(s.positions.back());
		data[&Data::edges].push_back(Edge{from, to, move(s)});
	}

	foreach (n : nodenums(*this))
		compute_in_out(n);

	data.forget_past();
}

vector<string> lines(string const & s)
{
	vector<string> v;
	boost::algorithm::split(v, s, [](char c){ return c == '\n'; });
	return v;
}

void Graph::set_description(NodeNum n, string const & d)
{
	data[n][&Node::description] = lines(d);
	data[n][&Node::dirty] = true;

}

void Graph::set_description(SeqNum s, string const & d)
{
	auto const v = lines(d);
	data[s][&Edge::description] = v;
	data[s][&Edge::dirty] = true;
	data[s][&Edge::detailed] = (properties_in_desc(v).count("detailed") != 0);
	data[s][&Edge::bidirectional] = (properties_in_desc(v).count("bidirectional") != 0);
}

}
