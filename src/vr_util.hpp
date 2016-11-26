#ifndef GRAPPLEMAP_VR_UTIL_HPP
#define GRAPPLEMAP_VR_UTIL_HPP

#include "math.hpp"

namespace GrappleMap
{
	inline V3 v3(Geometry::Vector<double, 3> const & v)
	{
		return V3{v[0], v[1], v[2]};
	}
}

#endif
