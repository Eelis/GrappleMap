#include "viables.hpp"
#include "graph_util.hpp"
#include "camera.hpp"

namespace GrappleMap {

namespace
{
	Viable viableFront(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const * const camera, PositionReorientation const & r)
	{
		auto const xyz = apply(r, graph[seq].positions.front(), j);
		auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
		return Viable{seq, r, 0, 1, xyz, xyz, xy, xy};
	}

	Viable viableBack(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const * const camera, PositionReorientation const & r)
	{
		auto const & sequence = graph[seq];
		auto const xyz = apply(r, sequence.positions.back(), j);
		auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
		return Viable{seq, r, end(sequence) - 1, end(sequence), xyz, xyz, xy, xy};
	}

	void extend_from(int depth, Graph const &, ReorientedNode, PlayerJoint, Camera const *, ViablesForJoint &);

	void extend_forward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		auto const & sequence = graph[via.seqNum];

		for (; via.end != sequence.positions.size(); ++via.end)
		{
			V3 const v = apply(via.reorientation, sequence.positions[via.end], j);

			if (distanceSquared(v, via.endV3) < 0.003) break;

			if (camera)
			{
				V2 const xy = world2xy(*camera, v);

				double const
					xydistSqrd = distanceSquared(xy, via.endxy),
					xydist = std::sqrt(xydistSqrd);

				if (xydist < 0.0005) break;

				via.endxy = xy;
				vfj.total_dist += xydist;
			}
			else ++vfj.total_dist; // in vr mode, just count segments

			via.endV3 = v;
		}

		if (via.end == sequence.positions.size())
		{
			auto const & to = graph.to(via.seqNum);
			ReorientedNode const n{to.node, compose(to.reorientation, via.reorientation)};

			assert(basicallySame(graph[n], via.reorientation(sequence.positions.back())));

			extend_from(depth + 1, graph, n, j, camera, vfj);
		}
	}

	void extend_backward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		auto & sequence = graph[via.seqNum];

		int pos = via.begin;
		--pos;
		for (; pos != -1; --pos)
		{
			V3 const v = apply(via.reorientation, sequence.positions[pos], j);

			if (distanceSquared(v, via.beginV3) < 0.003) break;

			if (camera)
			{
				V2 const xy = world2xy(*camera, v);

				double const
					xydistSqrd = distanceSquared(xy, via.beginxy),
					xydist = std::sqrt(xydistSqrd);

				if (xydist < 0.0005) break;

				via.beginxy = xy;
				vfj.total_dist += xydist;
			}
			else ++vfj.total_dist;

			via.beginV3 = v;
		}

		via.begin = pos + 1;

		if (via.begin == 0)
		{
			auto const & from = graph.from(via.seqNum);
			ReorientedNode const n{from.node, compose(from.reorientation, via.reorientation)};

			assert(basicallySame(graph[n], via.reorientation(sequence.positions.front())));

			extend_from(depth+1, graph, n, j, camera, vfj);
		}
	}

	void extend_from(int const depth, Graph const & graph, ReorientedNode const rn,
		PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		if (depth >= 2) return;

		foreach(seqNum : seqnums(graph))
		{
			#ifndef NDEBUG
				auto const & s = graph[seqNum];
				auto const here = graph[rn];
			#endif

			auto const from = graph.from(seqNum);
			auto const to = graph.to(seqNum);

			if (from.node == rn.node && vfj.viables.find(seqNum) == vfj.viables.end())
			{
				assert(basicallySame(graph[from], s.positions.front()));

				PositionReorientation const seqReo = compose(inverse(from.reorientation), rn.reorientation);

				assert(basicallySame(here, seqReo(s.positions.front())));

				extend_forward(
					depth + 1, graph,
					vfj.viables[seqNum] = viableFront(graph, seqNum, j, camera, seqReo),
					j, camera, vfj);
			}
			else if (to.node == rn.node && vfj.viables.find(seqNum) == vfj.viables.end())
			{
				assert(basicallySame(graph[to], s.positions.back()));

				PositionReorientation const seqReo = compose(inverse(to.reorientation), rn.reorientation);

				assert(basicallySame(here, seqReo(s.positions.back())));

				extend_backward(
					depth + 1, graph,
					vfj.viables[seqNum] = viableBack(graph, seqNum, j, camera, seqReo),
					j, camera, vfj);
			}
		}
	}
}

ViablesForJoint determineViables
	( Graph const & graph, PositionInSequence const from, PlayerJoint const j
	, bool const edit_mode, Camera const * const camera, PositionReorientation const reo)
{
	ViablesForJoint r
		{ 0
		, std::map<SeqNum, Viable>{}
		, std::vector<LineSegment>{} };

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = apply(reo, graph[from], j);
	auto const jpxy = camera ? world2xy(*camera, jp) : V2{0, 0};

	auto & v = r.viables[from.sequence] =
		Viable{from.sequence, reo, from.position, from.position + 1, jp, jp, jpxy, jpxy};
	extend_forward(0, graph, v, j, camera, r);
	extend_backward(0, graph, v, j, camera, r);

	if (r.total_dist < 0.3) return
		ViablesForJoint
			{ 0
			, std::map<SeqNum, Viable>{}
			, std::vector<LineSegment>{} };

	return r;
}

}
