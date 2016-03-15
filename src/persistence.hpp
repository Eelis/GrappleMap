#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include "graph_util.hpp"

ostream & operator<<(ostream &, Position const &);
ostream & operator<<(ostream &, Sequence const &);
Graph loadGraph(string filename);
void save(Graph const &, string filename);
Path readScene(Graph const &, string filename);
Position decodePosition(string);
void todot(Graph const &, std::ostream &, std::map<NodeNum, bool /* highlight */> const &, char heading);
void tojs(Graph const &, std::ostream &);

#endif
