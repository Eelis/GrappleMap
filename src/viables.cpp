#include "viables.hpp"
#include "graph_util.hpp"
#include "camera.hpp"

namespace GrappleMap {

namespace
{
	struct Calculator
	{
		Graph const & graph;
		PlayerJoint const j;
		Camera const * const camera;
		ViablesForJoint vfj;

		bool seen(SeqNum const s) const
		{
			foreach(x : vfj.viables)
				if (*x.sequence == s)
					return true;
			return false;
		}

		Viable viableFront(Reoriented<SeqNum> const seq)
		{
			auto const xyz = apply(seq.reorientation, graph[*seq].positions.front(), j);
			auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
			return Viable{seq, 0, 1, xyz, xyz, xy, xy};
		}

		Viable viableBack(Reoriented<SeqNum> const seq)
		{
			auto const & sequence = graph[*seq];
			auto const xyz = apply(seq.reorientation, sequence.positions.back(), j);
			auto const xy = camera ? world2xy(*camera, xyz) : V2{0, 0};
			return Viable{seq, last_pos(sequence), end(sequence), xyz, xyz, xy, xy};
		}

		void extend_forward(int const depth, Viable & via)
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
				extend_from(depth + 1, to(via.sequence, graph));
		}

		void extend_backward(int const depth, Viable & via)
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
				extend_from(depth+1, from(via.sequence, graph));
		}

		Reoriented<SeqNum> forget_direction(Reoriented<Reversible<SeqNum>> const & s)
		{
			return {**s, s.reorientation};
		}

		void extend_from(int const depth, ReorientedNode const rn)
		{
			if (depth >= 3) return;

			for (Reoriented<Reversible<SeqNum>> const & seq : out_sequences(rn, graph))
				if (!seq->reverse && !seen(**seq))
				{
					vfj.viables.push_back(viableFront(forget_direction(seq)));
					extend_forward(depth + 1, vfj.viables.back());
				}

			for (Reoriented<Reversible<SeqNum>> const & seq : in_sequences(rn, graph))
				if (!seq->reverse && !seen(**seq))
				{
					vfj.viables.push_back(viableBack(forget_direction(seq)));
					extend_backward(depth + 1, vfj.viables.back());
				}
		}
	};
}

ViablesForJoint determineViables
	( Graph const & graph, Reoriented<PositionInSequence> const from
	, PlayerJoint const j, Camera const * const camera)
{
	auto const jp = apply(from.reorientation, graph[*from], j);
	auto const jpxy = camera ? world2xy(*camera, jp) : V2{0, 0};

	Calculator c
		{	graph, j, camera
		,	ViablesForJoint
			{ 0
			, {Viable{sequence(from), from->position, next(from->position), jp, jp, jpxy, jpxy}}
			, std::vector<LineSegment>{}
			}
		};

	auto & v = c.vfj.viables.back();
	c.extend_forward(0, v);
	c.extend_backward(0, v);

	if (c.vfj.total_dist < 0.3) return ViablesForJoint{0, {}, {}};

	return c.vfj;
}

}
