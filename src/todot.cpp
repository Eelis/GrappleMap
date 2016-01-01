#include "util.hpp"
#include "persistence.hpp"
#include "graph_util.hpp"
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/range/algorithm_ext/insert.hpp>

struct Config
{
	string db;
	uint16_t depth;
	optional<NodeNum> start;
	optional<string> tag;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help", "show this help")
		("db", po::value<std::string>()->default_value("GrappleMap.txt"), "database file")
		("start", po::value<uint16_t>(), "start from this node")
		("depth", po::value<uint16_t>()->default_value(2), "include nodes up to this distance away from starting nodes")
		("tag", po::value<string>(), "start from nodes with this tag");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { cout << desc << '\n'; return none; }

	Config cfg
		{ vm["db"].as<string>()
		, vm["depth"].as<uint16_t>()
		, {}, {} };

	if (vm.count("start")) cfg.start = NodeNum{vm["start"].as<uint16_t>()};
	if (vm.count("tag")) cfg.tag = vm["tag"].as<string>();

	return cfg;
}

int main(int const argc, char const * const * const argv)
{
	if (auto config = config_from_args(argc, argv))
	{
		Graph const g = loadGraph(config->db);

		set<NodeNum> nodes;

		if (config->start)
			nodes = set<NodeNum>{*config->start};
		else if (config->tag)
			foreach(n : tagged_nodes(g, *config->tag)) nodes.insert(n);
				// todo: use boost's recent boost::range::insert that can insert into a set
		else
			nodes = set<NodeNum>(nodenums(g).begin(), nodenums(g).end());

		nodes = grow(g, nodes, config->depth);

		map<NodeNum, bool> m;
		foreach(n : nodes) m[n] = false;

		todot(g, cout, m, 'n');
	}
}
