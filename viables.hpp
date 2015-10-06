#ifndef JIUJITSUMAPPER_VIABLES_HPP
#define JIUJITSUMAPPER_VIABLES_HPP

#include "camera.hpp"
#include "positions.hpp"
#include <map>

struct Viable
{
	SeqNum seqNum;
	Reorientation r;
	double dist;
	PosNum begin, end; // half-open range, never empty
	V3 beginV3, endV3;
	V2 beginxy, endxy;
};

struct ViablesForJoint
{
	double total_dist;
	std::map<SeqNum, Viable> viables;
};

Viable viableFront(Graph const & graph, SeqNum const seq, PlayerJoint j, Camera const & camera, Reorientation const & r)
{
	auto xyz = apply(r, graph.edges[seq].sequence.positions.front()[j]);
	auto xy = world2xy(camera, xyz);
	return Viable{seq, r, 0, 0, 1, xyz, xyz, xy, xy};
}

Viable viableBack(Graph const & graph, SeqNum const seq, PlayerJoint j, Camera const & camera, Reorientation const & r)
{
	auto const & sequence = graph.edges[seq].sequence;
	auto xyz = apply(r, sequence.positions.back()[j]);
	auto xy = world2xy(camera, xyz);
	return Viable{seq, r, 0, end(sequence) - 1, end(sequence), xyz, xyz, xy, xy};
}

void extend_forward(int depth, Graph const &,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &);
void extend_backward(int depth, Graph const &,
	Viable &, PlayerJoint, Camera const &, ViablesForJoint &);

void extend_from(int const depth, Graph const & graph, ReorientedNode const rn,
	PlayerJoint const j, Camera const & camera, ViablesForJoint & vfj)
{
	if (depth > 3) return;

	for (auto && edge : graph.edges)
	{
		auto const & s = edge.sequence;

		auto const here = apply(rn.reorientation, graph.nodes[rn.node]);

		if (edge.from.node == rn.node && vfj.viables.find(edge.seqNum) == vfj.viables.end())
		{
			assert(basicallySame(get(graph, edge.from), s.positions.front()));

			extend_forward(
				depth + 1, graph,
				vfj.viables[edge.seqNum] = viableFront(
					graph, edge.seqNum, j, camera, compose(rn.reorientation, inverse(edge.from.reorientation))),
				j, camera, vfj);
		}
		else if (edge.to.node == rn.node && vfj.viables.find(edge.seqNum) == vfj.viables.end())
		{
			assert(basicallySame(get(graph, edge.to), s.positions.back()));

			auto const there = apply(compose(rn.reorientation, inverse(edge.to.reorientation)), s.positions.back());

			assert(basicallySame(here, there));

			extend_backward(
				depth + 1, graph,
				vfj.viables[edge.seqNum] = viableBack(
					graph, edge.seqNum, j, camera, compose(rn.reorientation, inverse(edge.to.reorientation))),
				j, camera, vfj);
		}
	}
}

void extend_forward(int const depth, Graph const & graph,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
{
	auto const & sequence = graph.edges[via.seqNum].sequence;

	for (; via.end != sequence.positions.size(); ++via.end)
	{
		V3 const v = apply(via.r, sequence.positions[via.end][j]);
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
	{
		auto & to = graph.edges[via.seqNum].to;
		ReorientedNode const n{to.node, compose(to.reorientation, via.r)};

		assert(basicallySame(get(graph, n), apply(via.r, sequence.positions.back())));

		extend_from(depth + 1, graph, n, j, camera, vfj);
	}
}

void extend_backward(int const depth, Graph const & graph,
	Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
{
	auto & sequence = graph.edges[via.seqNum].sequence;

	int pos = via.begin;
	--pos;
	for (; pos != -1; --pos)
	{
		V3 const v = apply(via.r, sequence.positions[pos][j]);
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
	{
		auto & from = graph.edges[via.seqNum].from;
		ReorientedNode const n{from.node, compose(from.reorientation, via.r)};

		assert(basicallySame(get(graph, n), apply(via.r, sequence.positions.front())));

		extend_from(depth+1, graph, n, j, camera, vfj);
	}
}

ViablesForJoint determineViables
	( Graph const & graph, PlayerJoint const j
	, SeqNum const seq, PosNum const pos
	, bool const edit_mode, Camera const & camera, Reorientation const reo)
{
	ViablesForJoint r{0, {}};

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = apply(reo, graph.edges[seq].sequence.positions[pos][j]);
	auto const jpxy = world2xy(camera, jp);

	auto & v = r.viables[seq] = Viable{seq, reo, 0, pos, pos+1, jp, jp, jpxy, jpxy};
	extend_forward(0, graph, v, j, camera, r);
	extend_backward(0, graph, v, j, camera, r);

	if (r.total_dist < 0.3) return {0, {}};

	return r;
}

#endif
