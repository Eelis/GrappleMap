#ifndef GRAPPLEMAP_PATHS_HPP
#define GRAPPLEMAP_PATHS_HPP

#include "graph_util.hpp"

namespace GrappleMap
{
	using Frames = vector<pair<string, vector<Position>>>;

	Frames smoothen(Frames);

	Frames frames(Graph const &, Path const &, unsigned frames_per_pos);

	Frames frames(Graph const &, vector<Path> const & script, unsigned frames_per_pos);

	Path randomScene(Graph const &, NodeNum start, size_t);

	vector<Path> paths_through(Graph const &, Step, unsigned in_size, unsigned out_size);

	Frames demoFrames(Graph const &, Step, unsigned frames_per_pos);
}

#endif
