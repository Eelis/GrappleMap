#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include "util.hpp"
#include "positions.hpp"

struct Graph;

istream & operator>>(istream &, vector<Sequence> &);
ostream & operator<<(ostream &, Position const &);
ostream & operator<<(ostream &, Sequence const &);
vector<Sequence> load(string filename);
void save(Graph const &, string filename);
vector<SeqNum> readScript(Graph const &, string filename);

#endif
