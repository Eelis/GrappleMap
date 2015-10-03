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
	std::map<unsigned /* sequence */, Viable> viables;
};

Viable viableFront(Sequence const & sequence, PlayerJoint j, Camera const & camera)
{
	auto xyz = sequence.positions.front()[j];
	auto xy = world2xy(camera, xyz);
	return Viable{0, 0, 1, xyz, xyz, xy, xy};
}

Viable viableBack(Sequence const & seq, PlayerJoint j, Camera const & camera)
{
	auto xyz = seq.positions.back()[j];
	auto xy = world2xy(camera, xyz);
	return Viable{0, end(seq) - 1, end(seq), xyz, xyz, xy, xy};
}

void extend_forward(
	std::vector<Sequence> const &, Graph const & graph, unsigned seqIndex,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &);
void extend_backward(
	std::vector<Sequence> const &, Graph const & graph, unsigned seqIndex,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &);

void extend_from(
	std::vector<Sequence> const & sequences, Graph const & graph, Position const & p,
	PlayerJoint j,
	Camera const & camera, ViablesForJoint & vfj)
{
	for (auto && edge : graph.edges)
	{
		if (vfj.viables.find(edge.sequence) != vfj.viables.end()) continue;

		auto & from = graph.nodes[edge.from];
		auto & to = graph.nodes[edge.to];
		auto & s = sequences[edge.sequence];

		if (from == p)
			extend_forward(
				sequences, graph, edge.sequence,
				vfj.viables[edge.sequence] = viableFront(s, j, camera),
				j, camera, vfj);
		else if (to == p)
			extend_backward(
				sequences, graph, edge.sequence,
				vfj.viables[edge.sequence] = viableBack(s, j, camera),
				j, camera, vfj);
	}
}

void extend_forward(
	std::vector<Sequence> const & sequences, Graph const & graph, unsigned const seqIndex,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
{
	auto & sequence = sequences[seqIndex];

	for (; via.end != sequence.positions.size(); ++via.end)
	{
		V3 const v = sequence.positions[via.end][j];
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

	if (via.end == sequence.positions.size())
		extend_from(sequences, graph, sequence.positions.back(), j, camera, vfj);
}

void extend_backward(
	std::vector<Sequence> const & sequences,  Graph const & graph, unsigned const seqIndex,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
{
	auto & sequence = sequences[seqIndex];

	int pos = via.begin;
	--pos;
	for (; pos != -1; --pos)
	{
		V3 const v = sequence.positions[pos][j];
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

	if (via.begin == 0)
		extend_from(sequences, graph, sequence.positions.front(), j, camera, vfj);
}

ViablesForJoint determineViables
	( std::vector<Sequence> const & sequences
	, Graph const & graph, PlayerJoint const j
	, unsigned const seq, unsigned const pos
	, bool edit_mode, Camera const & camera)
{
	assert(pos != s.positions.size());

	ViablesForJoint r{0, {}};

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = sequences[seq].positions[pos][j];
	auto const jpxy = world2xy(camera, jp);

	auto & v = r.viables[seq] = Viable{0, pos, pos+1, jp, jp, jpxy, jpxy};
	extend_forward(sequences, graph, seq, v, j, camera, r);
	extend_backward(sequences, graph, seq, v, j, camera, r);

	if (r.total_dist < 0.3) return {0, {}};

	return r;
}

#endif
