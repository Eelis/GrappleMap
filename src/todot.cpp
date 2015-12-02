#include "util.hpp"
#include "persistence.hpp"
#include "graph.hpp"
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>

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
