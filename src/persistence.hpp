#ifndef GRAPPLEMAP_PERSISTENCE_HPP
#define GRAPPLEMAP_PERSISTENCE_HPP

#include "paths.hpp"

namespace GrappleMap
{
	Graph loadGraph(std::istream &);
	Graph loadGraph(string filename);
	void save(Graph const &, string filename);
	Path readScene(Graph const &, string filename);
	void todot(Graph const &, std::ostream &, std::map<NodeNum, bool /* highlight */> const &, char heading);
	void tojs(PositionReorientation const &, std::ostream &);
	void tojs(Graph const &, std::ostream &);
}

#endif
