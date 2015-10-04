#ifndef JIUJITSUMAPPER_VIABLES_HPP
#define JIUJITSUMAPPER_VIABLES_HPP

#include "camera.hpp"
#include "positions.hpp"
#include <map>

struct Viable
{
	double dist;
	unsigned begin, end; // half-open range, never empty
	V3 beginV3, endV3;
	V2 beginxy, endxy;
};

struct ViablesForJoint
{
	double total_dist;
	std::map<std::pair<SeqNum, V3 /* offset */>, Viable> viables;
};

Viable viableFront(Sequence const & sequence, PlayerJoint j, Camera const & camera, V3 const off)
{
	auto xyz = sequence.positions.front()[j] + off;
	auto xy = world2xy(camera, xyz);
	return Viable{0, 0, 1, xyz, xyz, xy, xy};
}

Viable viableBack(Sequence const & seq, PlayerJoint j, Camera const & camera, V3 const off)
{
	auto xyz = seq.positions.back()[j] + off;
	auto xy = world2xy(camera, xyz);
	return Viable{0, end(seq) - 1, end(seq), xyz, xyz, xy, xy};
}

void extend_forward(int depth,
	std::vector<Sequence> const &, Graph const & graph, unsigned seqIndex,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &, V3 off);
void extend_backward(int depth,
	std::vector<Sequence> const &, Graph const & graph, unsigned seqIndex,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &, V3 off);

void extend_from(
	int depth,
	std::vector<Sequence> const & sequences, Graph const & graph, ReorientedNode const rn,
	PlayerJoint j,
	Camera const & camera, ViablesForJoint & vfj)
{
	if (depth > 4) return;

	for (auto && edge : graph.edges)
	{
		auto & s = sequences[edge.sequence];

		assert(sequences[edge.sequence].positions.front() + edge.from.offset == graph.nodes[edge.from.node]);
		assert(basicallySame(
			sequences[edge.sequence].positions.back() + edge.to.offset,
			graph.nodes[edge.to.node]));

		auto const
			fromKey = std::make_pair(edge.sequence, rn.offset + edge.from.offset),
			toKey = std::make_pair(edge.sequence, rn.offset + edge.to.offset);

		if (edge.from.node == rn.node && vfj.viables.find(fromKey) == vfj.viables.end())
			extend_forward(
				depth + 1, sequences, graph, edge.sequence,
				vfj.viables[fromKey] = viableFront(s, j, camera, fromKey.second),
				j, camera, vfj, fromKey.second);

		if (edge.to.node == rn.node && vfj.viables.find(toKey) == vfj.viables.end())
			extend_backward(
				depth + 1, sequences, graph, edge.sequence,
				vfj.viables[toKey] = viableBack(s, j, camera, toKey.second),
				j, camera, vfj, toKey.second);
	}
}

void extend_forward(int depth,
	std::vector<Sequence> const & sequences, Graph const & graph, unsigned const seqIndex,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj, V3 const off)
{
	auto & sequence = sequences[seqIndex];

	for (; via.end != sequence.positions.size(); ++via.end)
	{
		V3 const v = sequence.positions[via.end][j] + off;
		V2 const xy = world2xy(camera, v);

		if (distanceSquared(v, via.endV3) < 0.003) break;

		double const
			xydistSqrd = distanceSquared(xy, via.endxy),
			xydist = std::sqrt(xydistSqrd);

		if (xydist < 0.0005) break;

		/*
		auto const segment = LineSegment(xy, via.endxy);
		for (auto && old : vfj.segments)
			if (lineSegmentsIntersect(segment, old))
				return via;
		vfj.segments.push_back(segment);
		*/

		via.dist += xydist;
		vfj.total_dist += xydist;
		via.endV3 = v;
		via.endxy = xy;
	}

	auto & to = graph.edges[seqIndex].to;

	if (via.end == sequence.positions.size())
		extend_from(depth+1, sequences, graph, ReorientedNode{to.node, off - to.offset}, j, camera, vfj);
}

void extend_backward(int depth,
	std::vector<Sequence> const & sequences, Graph const & graph, unsigned const seqIndex,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj, V3 const off)
{
	auto & sequence = sequences[seqIndex];

	int pos = via.begin;
	--pos;
	for (; pos != -1; --pos)
	{
		V3 const v = sequence.positions[pos][j] + off;
		V2 const xy = world2xy(camera, v);

		if (distanceSquared(v, via.beginV3) < 0.003) break;

		double const
			xydistSqrd = distanceSquared(xy, via.beginxy),
			xydist = std::sqrt(xydistSqrd);

		if (xydistSqrd < 0.0005) break;

		/*
		auto const segment = std::make_pair(xy, via.beginxy);
		for (auto && old : vfj.segments)
			if (lineSegmentsIntersect(old, segment))
				return via;
		vfj.segments.push_back(segment);
		*/

		via.dist += xydist;
		vfj.total_dist += xydist;
		via.beginV3 = v;
		via.beginxy = xy;
	}

	via.begin = pos + 1;

	auto & from = graph.edges[seqIndex].from;

	if (via.begin == 0)
		extend_from(depth+1, sequences, graph, ReorientedNode{from.node, off - from.offset}, j, camera, vfj);
}

ViablesForJoint determineViables
	( std::vector<Sequence> const & sequences
	, Graph const & graph, PlayerJoint const j
	, unsigned const seq, unsigned const pos
	, bool edit_mode, Camera const & camera)
{
	ViablesForJoint r{0, {}};

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = sequences[seq].positions[pos][j];
	auto const jpxy = world2xy(camera, jp);

	auto & v = r.viables[{seq, {0, 0, 0}}] = Viable{0, pos, pos+1, jp, jp, jpxy, jpxy};
	extend_forward(0, sequences, graph, seq, v, j, camera, r, {0, 0, 0});
	extend_backward(0, sequences, graph, seq, v, j, camera, r, {0, 0, 0});

	if (r.total_dist < 0.3) return {0, {}};

	return r;
}

#endif
