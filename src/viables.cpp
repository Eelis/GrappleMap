#include "viables.hpp"
#include "camera.hpp"

namespace GrappleMap {

namespace
{
	void extend(
		Graph const & graph,
		PlayerJoint const j,
		Camera const * const camera,
		vector<Viable> & viables,
		unsigned const depth,
		Reoriented<PositionInSequence> pp,
		bool const node_done)
			// if pp happens to denote a node, then if node_done is true,
			// other segments adjacent to that node are already taken care of
	{
		Viable via{j, sequence(pp), pp->position, next(pp->position), pp->position, depth};

		auto const & sequ = graph[pp->sequence];

		V3 const startv = at(via.sequence * via.origin, j, graph);

		V3 lastv = startv;
		optional<V2> lastxy;
		unsigned lastd = depth;

		auto progress = [&](V3 const v)
			{
				if (distanceSquared(v, lastv) < 0.003) return false;

				if (camera)
				{
					if (!lastxy) lastxy = world2xy(*camera, lastv);

					V2 const xy = world2xy(*camera, v);
					if (distanceSquared(xy, *lastxy) < 0.0001) return false;
					lastxy = xy;
				}

				lastv = v;

				return true;
			};

		for (; via.end.index != sequ.positions.size(); ++via.end)
		{
			if (!progress(apply(via.sequence.reorientation, sequ[via.end], j)))
				break;
			if (++lastd == 7) break;
		}

		lastv = startv;
		lastxy = boost::none;
		lastd = depth;

		for (; via.begin != PosNum{0}; --via.begin)
		{
			if (!progress(apply(via.sequence.reorientation, sequ.positions[via.begin.index - 1], j)))
				break;
			if (++lastd == 7) break;
		}

		viables.push_back(via);

		auto further = [&](Reoriented<NodeNum> const & n, unsigned const depth)
			{
				for (Reoriented<Reversible<SeqNum>> const & seq : inout_sequences(n, graph))
					if (**seq != *via.sequence)
						extend(graph, j, camera, viables,
							depth, first_pos_in(seq, graph), true);
			};

		if (via.end == end(sequ) && (via.end != next(pp->position) || !node_done))
			further(to(via.sequence, graph), via.depth(*prev(via.end)));

		if (via.begin == PosNum{0} && (pp->position != PosNum{0} || !node_done))
			further(from(via.sequence, graph), via.depth(via.begin));
	}
}

bool viable(Graph const & graph,
	Reoriented<SegmentInSequence> const & segment,
	PlayerJoint const j, Camera const * const camera)
{
	return
		(distanceSquared(
			graph[from(*segment)][j],
			graph[to(*segment)][j]) > 0.003)
		&&
		(!camera || distanceSquared(
			world2xy(*camera, at(from_pos(segment), j, graph)),
			world2xy(*camera, at(to_pos(segment), j, graph))) > 0.0001);
}

vector<Viable> determineViables
	( Graph const & graph, Reoriented<PositionInSequence> const from
	, PlayerJoint const j, Camera const * const camera)
{
	vector<Viable> r;
	extend(graph, j, camera, r, 0, from, false);
	return r;
}

PerPlayerJoint<vector<Reoriented<SegmentInSequence>>>
	closeCandidates(
		Graph const & graph, Reoriented<SegmentInSequence> const & current,
		Camera const * const camera, OrientedPath const * const selection)
{
	PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> r;

	foreach (candidate : make_vector(current) + neighbours(current, graph, true))
		if (!selection || elem(candidate->sequence, *selection))
			foreach (j : playerJoints)
				if (viable(graph, candidate, j, camera))
					r[j].push_back(candidate);

	return r;
}

}
