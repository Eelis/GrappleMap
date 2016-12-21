#include "graph_util.hpp"

namespace GrappleMap {

SeqNum insert(Graph & g, Sequence const & sequence)
{
	SeqNum const num{g.num_sequences()};
	g.set(none, sequence);
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

void split_at(Graph & g, PositionInSequence const pis)
{
	if (node(g, pis)) throw runtime_error("cannot split node");

	Sequence a = g[pis.sequence], b = a;
	
	a.positions.erase(a.positions.begin() + pis.position.index + 1, a.positions.end());
	b.positions.erase(b.positions.begin(), b.positions.begin() + pis.position.index);

	g.set(pis.sequence, a);
	g.set(none, b);
}

pair<vector<Position>, ReorientedNode> follow(Graph const & g, ReorientedNode const & n, SeqNum const s, unsigned const frames_per_pos)
{
	vector<Position> positions;
	ReorientedNode m;

	if (*g[s].from == *n)
	{
		PositionReorientation const r = compose(inverse(g[s].from.reorientation), n.reorientation);

		for (PositionInSequence location = first_pos_in(s);
			next(location, g);
			location = *next(location, g))
					// See GCC bug 68003 for the reason behind the DRY violation.

			for (unsigned howfar = 0; howfar <= frames_per_pos; ++howfar)
				positions.push_back(between(
					r(at(location, g)),
					r(at(*next(location, g), g)),
					howfar / double(frames_per_pos)));

		*m = *g[s].to;
		m.reorientation = compose(g[s].to.reorientation, r);
	}
	else if (*g[s].to == *n)
	{
		PositionReorientation const r = compose(inverse(g[s].to.reorientation), n.reorientation);

		for (PositionInSequence location = last_pos_in(s, g);
			prev(location);
			location = *prev(location))
					// See GCC bug 68003 for the reason behind the DRY violation.

			for (unsigned howfar = 0; howfar <= frames_per_pos; ++howfar)
				positions.push_back(between(
					r(at(location, g)),
					r(at(*prev(location), g)),
					howfar / double(frames_per_pos)));

		*m = *g[s].from;
		m.reorientation = compose(g[s].from.reorientation, r);
	}
	else throw std::runtime_error(
		"node " + std::to_string(n->index) + " is not connected to sequence " + std::to_string(s.index));

	assert(basicallySame(positions.front(), g[n]));
	assert(basicallySame(positions.back(), g[m]));

	// todo: rewrite this function using some of the newer graph segment/position browsing utilities

	return {positions, m};
}

ReorientedNode follow(Graph const & g, ReorientedNode const & n, SeqNum const s)
{
	if (*g[s].from == *n)
		return
			{ *g[s].to
			, compose(g[s].to.reorientation,
			  compose(inverse(g[s].from.reorientation),
			  n.reorientation)) };
	else if (*g[s].to == *n)
		return { *g[s].from
			, compose(g[s].from.reorientation,
			  compose(inverse(g[s].to.reorientation),
			  n.reorientation)) };
	else throw std::runtime_error(
		"node " + std::to_string(n->index) + " is not connected to sequence " + std::to_string(s.index));
}

NodeNum follow(Graph const & g, NodeNum const n, SeqNum const s)
{
	if (*g[s].from == n) return *g[s].to;
	if (*g[s].to == n) return *g[s].from;
	throw std::runtime_error(
		"node " + std::to_string(n.index) + " is not connected to sequence " + std::to_string(s.index));
}

set<string> tags_in_desc(vector<string> const & desc)
{
	set<string> r;

	string const decl = "tags:";

	foreach(line : desc)
	{
		if (line.substr(0, decl.size()) == decl)
		{
			std::istringstream iss(line.substr(decl.size()));
			string tag;
			while (iss >> tag) r.insert(tag);
		}
	}

	return r;
}

bool connected(Graph const & g, NodeNum const a, set<NodeNum> const & s)
{
	foreach(b : s)
		if (connected(g, a, b)) return true;

	return false;
}

bool connected(Graph const & g, NodeNum const a, NodeNum const b)
{
	foreach (s : g[a].in)
		if (*from(g, s) == b)
			return true;
	
	foreach (s : g[a].out)
		if (*to(g, s) == b)
			return true;

	return false;
}

std::set<NodeNum> grow(Graph const & g, std::set<NodeNum> const & nodes)
{
	auto r = nodes;

	foreach(n : nodenums(g))
		if (connected(g, n, nodes)) { r.insert(n); break; }

	return r;
}

std::set<NodeNum> grow(Graph const & g, std::set<NodeNum> nodes, unsigned const depth)
{
	for (unsigned d = 0; d != depth; ++d) nodes = grow(g, nodes);
	return nodes;
}

set<NodeNum> nodes_around(Graph const & g, set<NodeNum> const & nodes, unsigned depth)
{
	set<NodeNum> all = nodes;
	set<NodeNum> r;

	for (unsigned d = 0; d != depth; ++d)
	{
		set<NodeNum> const prev = all;

		foreach(n : nodenums(g))
			if (connected(g, n, prev) && all.count(n) == 0)
			{
				all.insert(n);
				r.insert(n);
			}
	}

	return r;
}

Reoriented<Reversible<SeqNum>> gp_connect(
	Reoriented<NodeNum> const & n,
	Reversible<SeqNum> const s, Graph const & g)
{
	Reoriented<Reversible<SeqNum>> r;

	if (*from(g, s) == *n)
		r = {s, compose(inverse(from(g, s).reorientation), n.reorientation)};
	else if (*to(g, s) == *n)
		r = {s, compose(inverse(to(g, s).reorientation), n.reorientation)};
	else assert(!"gp_connect");

	return r;
}

vector<Reoriented<SegmentInSequence>> segments_around(ReorientedNode const & n, Graph const & g)
{
	vector<Reoriented<SegmentInSequence>> r;

	foreach (x : in_segments(n, g))
		r.push_back(**x * x.reorientation);

	foreach (x : out_segments(n, g))
		r.push_back(**x * x.reorientation);
		
	return r;
}

vector<ReorientedSegment> neighbours(
	ReorientedSegment const & s, Graph const & g,
	bool const open /* include segments in neighbouring sequences */)
{
	vector<ReorientedSegment> n;

	if (s->segment != last_segment(s->sequence, g).segment) // forward
		n.push_back({next(*s), s.reorientation});
	else if (open)
		n += segments_around(to(sequence(s), g), g);

	if (auto x = prev(*s)) // backward
		n.push_back({*x, s.reorientation});
	else if (open)
		n += segments_around(from(sequence(s), g), g);

	return n;
}

}
