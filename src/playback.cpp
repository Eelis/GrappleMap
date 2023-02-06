#include "playback.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "metadata.hpp"
#include "paths.hpp"
#include <unistd.h>

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <iostream>
#include <vector>
#include <fstream>

namespace GrappleMap
{
	namespace po = boost::program_options;

	optional<PlaybackConfig> playbackConfig_from_args(int const argc, char const * const * const argv)
	{
		po::options_description desc("options");
		desc.add_options()
			("help,h", "show this help")
			("frames-per-pos", po::value<unsigned>()->default_value(11),
				"number of frames rendered per position")
			("script", po::value<string>()->default_value(string()),
				"script file")
			("start", po::value<string>()->default_value("staredown"), "initial position")
			("length", po::value<unsigned>()->default_value(50), "number of transitions")
			("dimensions", po::value<string>(), "window dimensions")
			("dump", po::value<string>(), "file to write sequence data to")
			("seed", po::value<uint32_t>(), "PRNG seed")
			("db", po::value<string>()->default_value("GrappleMap.txt"), "database file")
			("demo", po::value<string>(), "show all chains of three transitions that have the given transition in the middle");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { cout << desc << '\n'; return none; }

		optional<pair<unsigned, unsigned>> dimensions;
		if (vm.count("dimensions"))
		{
			string const dims = vm["dimensions"].as<string>();
			auto x = dims.find('x');
			if (x == dims.npos) throw runtime_error("invalid dimensions");
			dimensions = pair<unsigned, unsigned>(std::stoul(dims.substr(0, x)), std::stoul(dims.substr(x + 1)));
		}

		return PlaybackConfig
			{ vm["db"].as<string>()
			, vm["script"].as<string>()
			, vm["frames-per-pos"].as<unsigned>()
			, vm["length"].as<unsigned>()
			, vm["start"].as<string>()
			, optionalopt<string>(vm, "demo")
			, dimensions
			, optionalopt<string>(vm, "dump")
			, optionalopt<uint32_t>(vm, "seed")
			};
	}

	Frames prep_frames(
		PlaybackConfig const & config,
		Graph const & graph)
	{
		if (config.demo)
		{
			if (auto step = step_by_desc(graph, *config.demo))
				return demoFrames(graph, *step, config.frames_per_pos);
			else
				throw runtime_error("no such transition: " + *config.demo);
		}
		else if (!config.script.empty())
			return smoothen(frames(graph, readScene(graph, config.script), config.frames_per_pos));
		else if (optional<NodeNum> start = node_by_desc(graph, config.start))
		{
			Frames x = frames(graph, randomScene(graph, *start, config.num_transitions), config.frames_per_pos);

			auto & v = x.front().second;
			auto & w = x.back().second;
			auto const firstpos = v.front();
			auto const lastpos = w.back();
			v.insert(v.begin(), config.frames_per_pos * 5, firstpos);
			w.insert(w.end(), config.frames_per_pos * 15, lastpos);

			return smoothen(x);
		}
		else
			throw runtime_error("no such position/transition: " + config.start);
	}

	Playback::Playback(Graph const & g, OrientedPath p)
		 : graph(g), path(std::move(p))
	{
		reset();
		chaser = at(location(), graph);
	}

	void Playback::reset()
	{
		Reoriented<Reversible<SegmentInSequence>> const
			fs = first_segment(path.front(), graph);

		i = path.begin();
		segment = (*fs)->segment;
		howFar = from_loc(fs)->howFar;
		wait = 0;
		atEnd = false;
	}

	void Playback::progress(double seconds)
	{
		if (wait > seconds) { wait -= seconds; return; }

		seconds -= wait;
		wait = 0;

		if (atEnd) { reset(); wait = pause_before; return; }

		Reoriented<Reversible<SeqNum>> const & sn = *i;

		Sequence const & seq = graph[**sn];

		auto & hf = howFar;

		double const speed // in segments per second
			= seq.detailed ? 10 : 5;
				// 5 segments per second (= 12 frames per segment at 60 frames per second)

		bool finished_sequence = false;

		if (!sn->reverse)
		{
			hf += seconds * speed;
			if (hf < 1) return;

			seconds = (hf - 1) / speed;

			if (segment != last_segment(seq)) { ++segment; hf = 0; }
			else { hf = 1; finished_sequence = true; }
		}
		else
		{
			hf -= seconds * speed;
			if (hf > 0) return;

			seconds = (- hf) / speed;

			if (segment.index != 0) { --segment.index; hf = 1; }
			else { hf = 0; finished_sequence = true; }
		}

		if (finished_sequence)
		{
			if (std::next(i) == path.end())
			{
				atEnd = true;
				wait = pause_after;
				return;
			}

			++i;

			if ((*i)->reverse)
			{
				segment = last_segment(graph[***i]);
				hf = 1;
			}
			else { segment.index = 0; hf = 0; }
		}

		progress(seconds);
	}

	void Playback::frame(double const secondsElapsed)
	{
		chaser = between(chaser, at(location(), graph), 0.2);
			// todo: the 0.2 should depend on secondsElapsed

		progress(secondsElapsed);
	}


}
