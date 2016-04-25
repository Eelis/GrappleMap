#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include "graph_util.hpp"

Graph loadGraph(string filename);
void save(Graph const &, string filename);
Path readScene(Graph const &, string filename);
void todot(Graph const &, std::ostream &, std::map<NodeNum, bool /* highlight */> const &, char heading);
void tojs(Graph const &, std::ostream &);

#endif
