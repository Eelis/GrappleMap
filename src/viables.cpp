#include "viables.hpp"
#include "graph_util.hpp"
#include "camera.hpp"

namespace GrappleMap {

namespace
{
	Viable viableFront(
		Graph const & graph, Reoriented<SeqNum> const seq,
		PlayerJoint const j, Camera const * const camera)
	{
		auto const xyz = apply(seq.reorientation, graph[*seq].positions.front(), j);
		auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
		return Viable{seq, 0, 1, xyz, xyz, xy, xy};
	}

	Viable viableBack(
		Graph const & graph, Reoriented<SeqNum> const seq,
		PlayerJoint const j, Camera const * const camera)
	{
		auto const & sequence = graph[*seq];
		auto const xyz = apply(seq.reorientation, sequence.positions.back(), j);
		auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
		return Viable{seq, last_pos(sequence), end(sequence), xyz, xyz, xy, xy};
	}

	void extend_from(int depth, Graph const &, ReorientedNode, PlayerJoint, Camera const *, ViablesForJoint &);

	void extend_forward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		auto const & sequence = graph[*via.sequence];

		for (; via.end.index != sequence.positions.size(); ++via.end)
		{
			V3 const v = apply(via.sequence.reorientation, sequence[via.end], j);

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

		if (via.end == end(sequence))
			extend_from(depth + 1, graph, to(via.sequence, graph), j, camera, vfj);
	}

	void extend_backward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		auto & sequence = graph[*via.sequence];

		int pos = via.begin.index;
		--pos;
		for (; pos != -1; --pos)
		{
			V3 const v = apply(via.sequence.reorientation, sequence.positions[pos], j);

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

		via.begin.index = pos + 1;

		if (via.begin.index == 0)
			extend_from(depth+1, graph, from(via.sequence, graph), j, camera, vfj);
	}

	Reoriented<SeqNum> forget_direction(Reoriented<Reversible<SeqNum>> const & s)
	{
		return {**s, s.reorientation};
	}

	void extend_from(int const depth, Graph const & graph, ReorientedNode const rn,
		PlayerJoint const j, Camera const * const camera, ViablesForJoint & vfj)
	{
		if (depth >= 2) return;

		for (Reoriented<Reversible<SeqNum>> const & seq : out_sequences(rn, graph))
			if (!seq->reverse && vfj.viables.find(**seq) == vfj.viables.end())
			{
				Reoriented<SeqNum> const x = forget_direction(seq);

				extend_forward(
					depth + 1, graph,
					vfj.viables[*x] = viableFront(graph, x, j, camera),
					j, camera, vfj);
			}

		for (Reoriented<Reversible<SeqNum>> const & seq : in_sequences(rn, graph))
			if (!seq->reverse && vfj.viables.find(**seq) == vfj.viables.end())
			{
				Reoriented<SeqNum> const x = forget_direction(seq);

				extend_backward(
					depth + 1, graph,
					vfj.viables[*x] = viableBack(graph, x, j, camera),
					j, camera, vfj);
			}
	}
}

ViablesForJoint determineViables
	( Graph const & graph, Reoriented<PositionInSequence> const from
	, PlayerJoint const j, Camera const * const camera)
{
	ViablesForJoint r
		{ 0
		, std::map<SeqNum, Viable>{}
		, std::vector<LineSegment>{} };

	auto const jp = apply(from.reorientation, graph[*from], j);
	auto const jpxy = camera ? world2xy(*camera, jp) : V2{0, 0};

	auto & v = r.viables[from->sequence] =
		Viable{sequence(from), from->position, next(from->position), jp, jp, jpxy, jpxy};
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
