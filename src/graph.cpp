#include "util.hpp"
#include "graph.hpp"

Graph::Graph(vector<Node> const & nodes, vector<Sequence> const & sequences)
{
	foreach (n : nodes) insert(n);
	foreach (s : sequences) ::insert(*this, s);
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

SeqNum insert(Graph & g, Sequence const & sequence)
{
	SeqNum const num{g.num_sequences()};

	g.set(none, sequence);

	std::cerr <<
		g.from(num).node << " ---seq" << num.index << "(" << sequence.positions.size() << ")---> " << g.to(num).node <<
		": \"" << sequence.description.front() << "\"" << std::endl;

	return num;
}

optional<SeqNum> erase_sequence(Graph & g, SeqNum const sn)
{
	if (g.num_sequences() == 1)
	{
		std::cerr << "Cannot erase last sequence." << std::endl;
		return none;
	}

	auto const & seq = g[sn];

	std::cerr <<
		"Erasing sequence " << sn.index <<
		" (\"" << seq.description.front() << "\")"
		" and the " << seq.positions.size() << " positions in it." << std::endl;

	g.set(sn, none);

	return SeqNum{sn.index == 0 ? 0 : sn.index - 1};
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

optional<SeqNum> seq_by_desc(Graph const & graph, std::string const & desc)
{
	for (SeqNum seqNum{0}; seqNum.index != graph.num_sequences(); ++seqNum.index)
		if (graph[seqNum].description.front() == desc)
			return seqNum;
	
	return none;
}

optional<PositionInSequence> node_as_posinseq(Graph const & graph, NodeNum const node)
{
	for (SeqNum seqNum{0}; seqNum.index != graph.num_sequences(); ++seqNum.index)
	{
		if (graph.from(seqNum).node == node) return first_pos_in(seqNum);
		else if (graph.to(seqNum).node == node) return last_pos_in(graph, seqNum);
	}

	return none;
}

void split_at(Graph & g, PositionInSequence const pis)
{
	if (node(g, pis)) throw std::runtime_error("cannot split node");

	Sequence a = g[pis.sequence], b = a;
	
	a.positions.erase(a.positions.begin() + pis.position + 1, a.positions.end());
	b.positions.erase(b.positions.begin(), b.positions.begin() + pis.position);

	g.set(pis.sequence, a);
	g.set(none, b);
}

pair<vector<Position>, ReorientedNode> follow(Graph const & g, ReorientedNode const & n, SeqNum const s, unsigned const frames_per_pos)
{
	vector<Position> positions;
	ReorientedNode m;

	if (g.from(s).node == n.node)
	{
		PositionReorientation const r = compose(inverse(g.from(s).reorientation), n.reorientation);

		assert(basicallySame( 
			g[s].positions.front(),
			g[g.from(s)],
			g.from(s).reorientation(g[g.from(s).node].position),
			g.from(s).reorientation(g[n.node].position),
			g[first_pos_in(s)] ));

		assert(basicallySame(
			r(g[first_pos_in(s)]),
			r(g.from(s).reorientation(g[n.node].position)),
			compose(inverse(g.from(s).reorientation), n.reorientation)(g.from(s).reorientation(g[n.node].position)),
			n.reorientation(inverse(g.from(s).reorientation)(g.from(s).reorientation(g[n.node].position))),
			n.reorientation(g[n.node].position),
			g[n]
			));

		for (PositionInSequence location = first_pos_in(s);
			next(g, location);
			location = *next(g, location))
					// See GCC bug 68003 for the reason behind the DRY violation.

			for (unsigned howfar = 0; howfar <= frames_per_pos; ++howfar)
				positions.push_back(between(
					r(g[location]),
					r(g[*next(g, location)]),
					howfar / double(frames_per_pos)));

		m.node = g.to(s).node;
		m.reorientation = compose(g.to(s).reorientation, r);

		assert(basicallySame(
			positions.back(),
			r(g[g.to(s)]),
			r(g.to(s).reorientation(g[g.to(s).node].position)),
			r(g.to(s).reorientation(g[m.node].position)),
			r(g.to(s).reorientation(g[m.node].position)),
			compose(g.to(s).reorientation, r)(g[m.node].position),
			m.reorientation(g[m.node].position),
			m.reorientation(g[g.to(s).node].position),
			g[m]
			));
	}
	else if (g.to(s).node == n.node)
	{
		assert(basicallySame(
			g[s].positions.back(),
			g[g.to(s)],
			g.to(s).reorientation(g[g.to(s).node].position),
			g.to(s).reorientation(g[n.node].position),
			g[last_pos_in(g, s)]
			));

		PositionReorientation const r = compose(inverse(g.to(s).reorientation), n.reorientation);

		assert(basicallySame(
			r(g[last_pos_in(g, s)]),
			r(g.to(s).reorientation(g[n.node].position)),
			compose(inverse(g.to(s).reorientation), n.reorientation)(g.to(s).reorientation(g[n.node].position)),
			n.reorientation(inverse(g.to(s).reorientation)(g.to(s).reorientation(g[n.node].position))),
			n.reorientation(g[n.node].position),
			g[n]
			));

		for (PositionInSequence location = last_pos_in(g, s);
			prev(location);
			location = *prev(location))
					// See GCC bug 68003 for the reason behind the DRY violation.

			for (unsigned howfar = 0; howfar <= frames_per_pos; ++howfar)
				positions.push_back(between(
					r(g[location]),
					r(g[*prev(location)]),
					howfar / double(frames_per_pos)));

		m.node = g.from(s).node;
		m.reorientation = compose(g.from(s).reorientation, r);
	}
	else throw std::runtime_error(
		"node " + std::to_string(n.node.index) + " is not connected to sequence " + std::to_string(s.index));

	assert(basicallySame(positions.front(), g[n]));
	assert(basicallySame(positions.back(), g[m]));

	positions.pop_back();

	return {positions, m};
}

bool connected(Graph const & g, NodeNum const a, NodeNum const b)
{
	for (SeqNum s{0}; s.index != g.num_sequences(); ++s.index)
		if ((g.from(s).node == a && g.to(s).node == b) ||
		    (g.to(s).node == a && g.from(s).node == b))
			return true;

	return false;
}


std::set<NodeNum> nodes_around(Graph const & g, NodeNum const n, unsigned const depth)
{
	std::set<NodeNum> r{n};

	for (unsigned d = 0; d != depth; ++d)
	{
		auto prev = r;
		for (NodeNum n{0}; n.index != g.num_nodes(); ++n.index)
		{
			foreach (v : prev)
				if (connected(g, n, v)) { r.insert(n); break; }

		}
	}

	return r;
}
