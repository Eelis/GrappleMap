#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include "graph.hpp"

ostream & operator<<(ostream &, Position const &);
ostream & operator<<(ostream &, Sequence const &);
Graph loadGraph(string filename);
void save(Graph const &, string filename);
vector<SeqNum> readScript(Graph const &, string filename);
Position decodePosition(string);
void todot(Graph const &, std::ostream &, std::map<NodeNum, bool /* highlight */> const &);

#endif
