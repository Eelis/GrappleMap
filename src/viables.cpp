#include "viables.hpp"
#include "graph_util.hpp"
#include "camera.hpp"

namespace GrappleMap {

namespace
{
	Viable viableFront(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const & camera, PositionReorientation const & r)
	{
		auto const xyz = apply(r, graph[seq].positions.front(), j);
		auto const xy = world2xy(camera, xyz);
		return Viable{seq, r, 0, 0, 1, xyz, xyz, xy, xy};
	}

	Viable viableBack(
		Graph const & graph, SeqNum const seq, PlayerJoint const j,
		Camera const & camera, PositionReorientation const & r)
	{
		auto const & sequence = graph[seq];
		auto const xyz = apply(r, sequence.positions.back(), j);
		auto const xy = world2xy(camera, xyz);
		return Viable{seq, r, 0, end(sequence) - 1, end(sequence), xyz, xyz, xy, xy};
	}

	void extend_from(int depth, Graph const &, ReorientedNode, PlayerJoint, Camera const &, ViablesForJoint &);

	void extend_forward(int const depth, Graph const & graph,
		Viable & via, PlayerJoint const j, Camera const & camera, ViablesForJoint & vfj)
	{
		auto const & sequence = graph[via.seqNum];

		for (; via.end != sequence.positions.size(); ++via.end)
		{
			V3 const v = apply(via.reorientation, sequence.positions[via.end], j);
			V2 const xy = world2xy(camera, v);

			if (distanceSquared(v, via.endV3) < 0.003) break;

			double const
				xydistSqrd = distanceSquared(xy, via.endxy),
				xydist = std::sqrt(xydistSqrd);

			if (xydist < 0.0005) break;
/*
			auto const segment = LineSegment(xy, via.endxy);
			foreach (old : vfj.segments) if (lineSegmentsIntersect(segment, old)) return;
			vfj.segments.push_back(segment);
*/
			via.dist += xydist;
			vfj.total_dist += xydist;
			via.endV3 = v;
			via.endxy = xy;
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
		Viable & via, PlayerJoint const j, Camera const & camera, ViablesForJoint & vfj)
	{
		auto & sequence = graph[via.seqNum];

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

			if (xydist < 0.0005) break;
/*
			auto const segment = make_pair(xy, via.beginxy);
			foreach (old : vfj.segments) if (lineSegmentsIntersect(old, segment)) return;
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
			auto const & from = graph.from(via.seqNum);
			ReorientedNode const n{from.node, compose(from.reorientation, via.reorientation)};

			assert(basicallySame(graph[n], via.reorientation(sequence.positions.front())));

			extend_from(depth+1, graph, n, j, camera, vfj);
		}
	}

	void extend_from(int const depth, Graph const & graph, ReorientedNode const rn,
		PlayerJoint const j, Camera const & camera, ViablesForJoint & vfj)
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
	, bool const edit_mode, Camera const & camera, PositionReorientation const reo)
{
	ViablesForJoint r
		{ 0
		, std::map<SeqNum, Viable>{}
		, std::vector<LineSegment>{} };

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = apply(reo, graph[from], j);
	auto const jpxy = world2xy(camera, jp);

	auto & v = r.viables[from.sequence] =
		Viable{from.sequence, reo, 0, from.position, from.position + 1, jp, jp, jpxy, jpxy};
	extend_forward(0, graph, v, j, camera, r);
	extend_backward(0, graph, v, j, camera, r);

	if (r.total_dist < 0.3) return
		ViablesForJoint
			{ 0
			, std::map<SeqNum, Viable>{}
			, std::vector<LineSegment>{} };

	return r;
}

namespace
{
	struct VrViableExtender
	{
		Graph const & graph;
		VrViablesForJoint & vfj;
		PlayerJoint const joint;

		VrViable viableVrFront(SeqNum const seq, PositionReorientation const & r) const
		{
			auto const xyz = apply(r, graph[seq].positions.front(), joint);
			return VrViable{seq, r, 0, 0, 1, xyz, xyz};
		}

		VrViable viableVrBack(SeqNum const seq, PositionReorientation const & r) const
		{
			auto const & sequence = graph[seq];
			auto const xyz = apply(r, sequence.positions.back(), joint);
			return VrViable{seq, r, 0, end(sequence) - 1, end(sequence), xyz, xyz};
		}

		void forward(int const depth, VrViable & via) const
		{
			auto const & sequence = graph[via.seqNum];
			auto const & to = graph.to(via.seqNum);

			for (; via.end != sequence.positions.size(); ++via.end)
			{
				V3 const v = apply(via.reorientation, sequence.positions[via.end], joint);

				if (distanceSquared(v, via.endV3) < 0.003) break;
				via.endV3 = v;
			}

			if (via.end == sequence.positions.size())
				efrom(depth + 1, ReorientedNode{to.node, compose(to.reorientation, via.reorientation)});
		}

		void backward(int const depth, VrViable & via) const
		{
			auto & sequence = graph[via.seqNum];
			auto const & from = graph.from(via.seqNum);

			int pos = via.begin;
			--pos;
			for (; pos != -1; --pos)
			{
				V3 const v = apply(via.reorientation, sequence.positions[pos], joint);

				if (distanceSquared(v, via.beginV3) < 0.003) break;

				via.beginV3 = v;
			}

			via.begin = pos + 1;

			if (via.begin == 0)
				efrom(depth + 1, ReorientedNode{from.node, compose(from.reorientation, via.reorientation)});
		}

		void efrom(int const depth, ReorientedNode const rn) const
		{
			if (depth >= 2) return;

			foreach(seqNum : seqnums(graph))
			{
				auto const from = graph.from(seqNum);
				auto const to = graph.to(seqNum);

				if (from.node == rn.node && !elem(seqNum, vfj))
				{
					auto seqReo = compose(inverse(from.reorientation), rn.reorientation);

					forward(depth + 1, vfj[seqNum] = viableVrFront(seqNum, seqReo));
				}
				else if (to.node == rn.node && !elem(seqNum, vfj))
				{
					auto seqReo = compose(inverse(to.reorientation), rn.reorientation);

					backward(depth + 1, vfj[seqNum] = viableVrBack(seqNum, seqReo));
				}
			}
		}
	};
}

VrViablesForJoint determineVrViables
	( Graph const & graph, PositionInSequence const from, PlayerJoint const j
	, bool const edit_mode, PositionReorientation const reo)
{
	VrViablesForJoint r;

	if (!edit_mode && !jointDefs[j.joint].draggable) return r;

	auto const jp = apply(reo, graph[from], j);

	auto & v = r[from.sequence] =
		VrViable{from.sequence, reo, 0, from.position, from.position + 1, jp, jp};

	VrViableExtender e{graph, r, j};

	e.forward(0, v);
	e.backward(0, v);

	return r;
}

}
