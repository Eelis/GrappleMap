#ifndef GRAPPLEMAP_VIABLES_HPP
#define GRAPPLEMAP_VIABLES_HPP

#include "paths.hpp"
#include <map>

namespace GrappleMap
{
	struct Graph;
	struct Camera;

	struct Viable
	{
		PlayerJoint joint;
		Reoriented<SeqNum> sequence;
		PosNum begin, end; // half-open range, never empty
		PosNum origin;
		unsigned origin_depth;

		unsigned depth(PosNum const p) const
		{
			return origin_depth + std::abs(p.index - origin.index);
		}
	};

	bool viable(
		Graph const &, Reoriented<SegmentInSequence> const &,
		PlayerJoint, Camera const *);

	vector<Viable> determineViables(
		Graph const &, Reoriented<PositionInSequence>,
		PlayerJoint, Camera const *);

	PerPlayerJoint<vector<Reoriented<SegmentInSequence>>>
		closeCandidates(
			Graph const &, Reoriented<SegmentInSequence> const &,
			Camera const *, OrientedPath const *);
}

#endif
