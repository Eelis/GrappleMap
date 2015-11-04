#include "util.hpp"
#include "persistence.hpp"
#include "graph.hpp"
#include <iostream>
#include <vector>

void todot(Graph const & graph, std::ostream & o)
{
	o << "digraph G {\n";

	for (SeqNum s{0}; s.index != graph.num_sequences(); ++s.index)
		o << graph.from(s).node << " -> " << graph.to(s).node
		  << " [label=\"" << graph[s].description << " (" << graph[s].positions.size()-2 << ")\"];\n";

	o << "}\n";
}

int main()
{
	std::vector<Sequence> sequences;
	std::cin >> sequences;
	todot(Graph(sequences), std::cout);
}
