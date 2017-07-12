#include "camera.hpp"
#include "persistence.hpp"
#include "positions.hpp"
#include "rendering.hpp"
#include "metadata.hpp"
#include "images.hpp"
#include "paths.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace GrappleMap;

struct Config
{
	string db;
	string script;
	unsigned frames_per_pos;
	unsigned num_transitions;
	string start;
	optional<string /* desc */> demo;
	pair<unsigned, unsigned> dimensions;
	optional<uint32_t> seed;
};

optional<Config> config_from_args(int const argc, char const * const * const argv)
{
	namespace po = boost::program_options;

	po::options_description desc("options");
	desc.add_options()
		("help,h", "show this help")
		("frames-per-pos", po::value<unsigned>()->default_value(11),
			"number of frames rendered per position")
		("script", po::value<string>()->default_value(string()),
			"script file")
		("start", po::value<string>()->default_value("staredown"), "initial position")
		("length", po::value<unsigned>()->default_value(50), "number of transitions")
		("dimensions", po::value<string>()->default_value("1280x720"), "video resolution")
		("seed", po::value<uint32_t>(), "PRNG seed")
		("db", po::value<string>()->default_value("GrappleMap.txt"), "database file")
		("demo", po::value<string>(), "show all chains of three transitions that have the given transition in the middle");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) { cout << desc << '\n'; return none; }

	string const dims = vm["dimensions"].as<string>();
	auto x = dims.find('x');
	if (x == dims.npos) throw runtime_error("invalid dimensions");
	pair<unsigned, unsigned> dimensions{std::stoul(dims.substr(0, x)), std::stoul(dims.substr(x + 1))};

	return Config
		{ vm["db"].as<string>()
		, vm["script"].as<string>()
		, vm["frames-per-pos"].as<unsigned>()
		, vm["length"].as<unsigned>()
		, vm["start"].as<string>()
		, vm.count("demo") ? optional<string>(vm["demo"].as<string>()) : boost::none
		, dimensions
		, optionalopt<uint32_t>(vm, "seed")
		};
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		std::srand(std::time(nullptr));

		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		std::srand(config->seed ? *config->seed : std::time(nullptr));

		Graph const graph = loadGraph(config->db);

		ImageMaker mkImg(graph, "TODO");

		Frames fr;

		if (config->demo)
		{
			if (auto step = step_by_desc(graph, *config->demo))
				fr = demoFrames(graph, *step, config->frames_per_pos);
			else
				throw runtime_error("no such transition: " + *config->demo);
		}
		else if (!config->script.empty())
			fr = smoothen(frames(graph, readScene(graph, config->script), config->frames_per_pos));
		else if (optional<NodeNum> start = node_by_desc(graph, config->start))
		{
			Frames x = frames(graph, randomScene(graph, *start, config->num_transitions), config->frames_per_pos);

			auto & v = x.front().second;
			auto & w = x.back().second;
			auto const firstpos = v.front();
			auto const lastpos = w.back();
			v.insert(v.begin(), config->frames_per_pos * 15, firstpos);
			w.insert(w.end(), config->frames_per_pos * 15, lastpos);

			fr = smoothen(x);
		}
		else
			throw runtime_error("no such position/transition: " + config->start);

		unsigned frameindex = 0;

		string const separator = "      ";

		Camera camera;
		camera.zoom(1.2);
		camera.hardSetOffset(cameraOffsetFor(fr.front().second.front()));

		for (auto i = fr.begin(); i != fr.end(); ++i)
		{
			foreach (pos : i->second)
			{
				camera.rotateHorizontal(-0.012);
				camera.setOffset(cameraOffsetFor(pos));
/* todo:
				int const
					width = config->dimensions.first,
					height = config->dimensions.second;

				std::ostringstream fn;
				fn << "vidframes/frame" << std::setw(5) << std::setfill('0') << frameindex << ".png";

				mkImg.png(pos, camera, width, height, fn.str(),
					white, // background
//					{{0, 0, 1, 1, none, 50}}, // view
					third_person_windows_in_corner(.3,.3,.01,.01 * (double(width)/height)),
					20, // grid size
					4 // grid line width
					);
*/
				++frameindex;
			}

			cout << (i - fr.begin()) << ' ' << std::flush;
		}

		cout << "\nGenerated " << frameindex << " frames.\n";
	}
	catch (exception const & e)
	{
		cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
