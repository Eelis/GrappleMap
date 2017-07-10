#include "persistence.hpp"
#include "js_conversions.hpp"
#include <boost/program_options.hpp>

using namespace GrappleMap;

struct Config
{
	string db;
	string output_dir;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help,h",
			"show this help")
		("output_dir",
			po::value<string>()->default_value("."),
			"output directory")
		("db",
			po::value<string>()->default_value("GrappleMap.txt"),
			"database file");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { std::cout << desc << '\n'; return none; }

	return Config
		{ vm["db"].as<string>()
		, vm["output_dir"].as<string>() };
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		ofstream js(config->output_dir + "/transitions.js");

		js << std::boolalpha;

		tojs(GrappleMap::loadGraph(config->db), js);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
