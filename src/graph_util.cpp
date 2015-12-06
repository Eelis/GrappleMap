#include "graph_util.hpp"

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

optional<SeqNum> seq_by_desc(Graph const & g, std::string const & desc)
{
	foreach(sn : seqnums(g))
		if (g[sn].description.front() == desc)
			return sn;
	
	return none;
}

optional<PositionInSequence> node_as_posinseq(Graph const & g, NodeNum const node)
{
	foreach(sn : seqnums(g))
		if (g.from(sn).node == node) return first_pos_in(sn);
		else if (g.to(sn).node == node) return last_pos_in(g, sn);

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
	foreach(s : seqnums(g))
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
		foreach(n : nodenums(g))
			foreach (v : prev)
				if (connected(g, n, v)) { r.insert(n); break; }
	}

	return r;
}
