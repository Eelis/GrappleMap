// From https://schneide.wordpress.com/2016/07/15/generating-an-icosphere-in-c/

#ifndef ICOSPHERE_HPP
#define ICOSPHERE_HPP

#include "math.hpp"
#include <vector>

namespace icosphere
{
	using Index = int;

	struct Triangle { Index vertex[3]; };

	using IndexedMesh = std::pair<std::vector<GrappleMap::V3>, std::vector<Triangle>>;

	IndexedMesh make_icosphere(int subdivisions);
}

#endif
