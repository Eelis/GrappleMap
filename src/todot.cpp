#include "util.hpp"
#include "persistence.hpp"
#include "graph.hpp"
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>

bool connected(Graph const & g, NodeNum const a, NodeNum const b)
{
	for (SeqNum s{0}; s.index != g.num_sequences(); ++s.index)
		if ((g.from(s).node == a && g.to(s).node == b) ||
		    (g.to(s).node == a && g.from(s).node == b))
			return true;

	return false;
}


std::set<NodeNum> nodes_around(Graph const & g, NodeNum const n, unsigned const depth)
{
	std::set<NodeNum> r{n};

	for (unsigned d = 0; d != depth; ++d)
	{
		auto prev = r;
		for (NodeNum n{0}; n.index != g.num_nodes(); ++n.index)
		{
			foreach (v : prev)
				if (connected(g, n, v)) { r.insert(n); break; }

		}
	}

	return r;
}

void todot(Graph const & graph, std::ostream & o, boost::optional<std::pair<NodeNum, unsigned /* depth */>> const focus = boost::none)
{
	std::set<NodeNum> visited;
/*
	if (focus)
		visited = nodes_around(graph, focus->first, focus->second);
	else
*/
		for (NodeNum n{0}; n.index != graph.num_nodes(); ++n.index)
			visited.insert(n);

	o << "digraph G {\n";

	foreach (n : visited)
		if (!graph[n].description.empty())
			o << n.index << " [label=\"" << graph[n].description.front() << " (" << n.index << ")\"];\n";

	for (SeqNum s{0}; s.index != graph.num_sequences(); ++s.index)
	{
		auto const
			from = graph.from(s).node,
			to = graph.to(s).node;

		if (!visited.count(from) || !visited.count(to)) continue;

		o << from.index << " -> " << to.index
		  << " [label=\"" << graph[s].description.front() << " (" << s.index << ", " << graph[s].positions.size()-2 << ")\"];\n";
	}

	o << "}\n";
}

struct Config
{
	string db;
	uint16_t start;
	uint16_t depth;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help", "show this help")
		("db", po::value<std::string>()->default_value("positions.txt"), "position database file")
		("start", po::value<uint16_t>()->default_value(0), "initial node")
		("depth", po::value<uint16_t>()->default_value(2), "include nodes up to this distance away from start");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { cout << desc << '\n'; return none; }

	return Config
		{ vm["db"].as<string>()
		, vm["start"].as<uint16_t>()
		, vm["depth"].as<uint16_t>() };
}

int main(int const argc, char const * const * const argv)
{
	if (auto config = config_from_args(argc, argv))
		todot(loadGraph(config->db), cout, make_pair(NodeNum{config->start}, unsigned(config->depth)));
}
