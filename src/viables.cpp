#include "viables.hpp"
#include "util.hpp"
#include "graph.hpp"
#include "camera.hpp"

namespace
{
	Viable viableFront(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const & camera, PositionReorientation const & r)
	{
		auto const xyz = apply(r, graph.sequence(seq).positions.front(), j);
		auto const xy = world2xy(camera, xyz);
		return Viable{seq, r, 0, 0, 1, xyz, xyz, xy, xy};
	}

	Viable viableBack(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const & camera, PositionReorientation const & r)
	{
		auto const & sequence = graph.sequence(seq);
		auto const xyz = apply(r, sequence.positions.back(), j);
		auto const xy = world2xy(camera, xyz);
		return Viable{seq, r, 0, end(sequence) - 1, end(sequence), xyz, xyz, xy, xy};
	}

	void extend_from(int depth, Graph const &, ReorientedNode, PlayerJoint, Camera const &, ViablesForJoint &);

	void extend_forward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
	{
		auto const & sequence = graph.sequence(via.seqNum);

		for (; via.end != sequence.positions.size(); ++via.end)
		{
			V3 const v = apply(via.reorientation, sequence.positions[via.end], j);
			V2 const xy = world2xy(camera, v);

			if (distanceSquared(v, via.endV3) < 0.003) break;

			double const
				xydistSqrd = distanceSquared(xy, via.endxy),
				xydist = std::sqrt(xydistSqrd);

			if (xydist < 0.0005) break;

			auto const segment = LineSegment(xy, via.endxy);
			foreach (old : vfj.segments) if (lineSegmentsIntersect(segment, old)) return;
			vfj.segments.push_back(segment);

			via.dist += xydist;
			vfj.total_dist += xydist;
			via.endV3 = v;
			via.endxy = xy;
		}

		if (via.end == sequence.positions.size())
		{
			auto const & to = graph.to(via.seqNum);
			ReorientedNode const n{to.node, compose(to.reorientation, via.reorientation)};

			assert(basicallySame(graph[n], apply(via.reorientation, sequence.positions.back())));

			extend_from(depth + 1, graph, n, j, camera, vfj);
		}
	}

	void extend_backward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint j, Camera const & camera, ViablesForJoint & vfj)
	{
		auto & sequence = graph.sequence(via.seqNum);

		int pos = via.begin;
		--pos;
		for (; pos != -1; --pos)
		{
			V3 const v = apply(via.reorientation, sequence.positions[pos], j);
			V2 const xy = world2xy(camera, v);

			if (distanceSquared(v, via.beginV3) < 0.003) break;

			double const
				xydistSqrd = distanceSquared(xy, via.beginxy),
				xydist = std::sqrt(xydistSqrd);

			if (xydistSqrd < 0.0005) break;

			auto const segment = std::make_pair(xy, via.beginxy);
			foreach (old : vfj.segments) if (lineSegmentsIntersect(old, segment)) return;
			vfj.segments.push_back(segment);

			via.dist += xydist;
			vfj.total_dist += xydist;
			via.beginV3 = v;
			via.beginxy = xy;
		}

		via.begin = pos + 1;

		if (via.begin == 0)
		{
			auto const & from = graph.from(via.seqNum);
			ReorientedNode const n{from.node, compose(from.reorientation, via.reorientation)};

			assert(basicallySame(graph[n], apply(via.reorientation, sequence.positions.front())));

			extend_from(depth+1, graph, n, j, camera, vfj);
		}
	}

	void extend_from(int const depth, Graph const & graph, ReorientedNode const rn,
		PlayerJoint const j, Camera const & camera, ViablesForJoint & vfj)
	{
		if (depth > 3) return;

		for (SeqNum seqNum = 0; seqNum != graph.num_sequences(); ++seqNum)
		{
			#ifndef NDEBUG
				auto const & s = graph.sequence(seqNum);
				auto const here = graph[rn];
			#endif

			auto const from = graph.from(seqNum);
			auto const to = graph.to(seqNum);

			if (from.node == rn.node && vfj.viables.find(seqNum) == vfj.viables.end())
			{
				assert(basicallySame(graph[from], s.positions.front()));

				PositionReorientation const seqReo = compose(inverse(from.reorientation), rn.reorientation);

				assert(basicallySame(here, apply(seqReo, s.positions.front())));

				extend_forward(
					depth + 1, graph,
					vfj.viables[seqNum] = viableFront(graph, seqNum, j, camera, seqReo),
					j, camera, vfj);
			}
			else if (to.node == rn.node && vfj.viables.find(seqNum) == vfj.viables.end())
			{
				assert(basicallySame(graph[to], s.positions.back()));

				PositionReorientation const seqReo = compose(inverse(to.reorientation), rn.reorientation);

				assert(basicallySame(here, apply(seqReo, s.positions.back())));

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
	, bool const edit_mode, Camera const & camera, PositionReorientation const reo)
{
	ViablesForJoint r{0, {}, {}};

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = apply(reo, graph[from], j);
	auto const jpxy = world2xy(camera, jp);

	auto & v = r.viables[from.sequence] =
		Viable{from.sequence, reo, 0, from.position, from.position + 1, jp, jp, jpxy, jpxy};
	extend_forward(0, graph, v, j, camera, r);
	extend_backward(0, graph, v, j, camera, r);

	if (r.total_dist < 0.3) return {0, {}, {}};

	return r;
}
