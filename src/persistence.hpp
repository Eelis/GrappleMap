#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include <vector>
#include <string>
#include "positions.hpp"

struct Graph;

std::istream & operator>>(std::istream &, std::vector<Sequence> &);
std::ostream & operator<<(std::ostream &, Position const &);
std::ostream & operator<<(std::ostream &, Sequence const &);
std::vector<Sequence> load(std::string filename);
void save(Graph const &, std::string filename);

#endif
