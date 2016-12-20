#ifndef GRAPPLEMAP_PLAYBACK_HPP
#define GRAPPLEMAP_PLAYBACK_HPP

#include "paths.hpp"

namespace GrappleMap
{
	struct PlaybackConfig
	{
		string db;
		string script;
		unsigned frames_per_pos;
		unsigned num_transitions;
		string start;
		optional<string /* desc */> demo;
		optional<pair<unsigned, unsigned>> dimensions;
		optional<string> dump;
		optional<uint32_t> seed;
	};

	optional<PlaybackConfig> playbackConfig_from_args(int argc, char const * const * argv);

	Frames prep_frames(PlaybackConfig const &, Graph const &);

	class Playback // also used by Editor
	{
		static constexpr double
			pause_before = 0.8,
			pause_after = 0.8;

		double wait = 0;
		bool atEnd = false;
		Graph const & graph;
		OrientedPath const path;
		OrientedPath::const_iterator i = path.begin();
		SegmentNum segment;
		double howFar;
		Position chaser;

		Playback(Playback const &) = delete;
		Playback(Playback &&) = delete;

	public:

		Reoriented<Location> location() const
		{
			assert (i != path.end());

			return {
				Location{{***i, segment}, howFar},
				i->reorientation};
		}

		Playback(Graph const &, OrientedPath const &);

		Position const & getPosition() const { return chaser; }

		void reset();
		void frame(double seconds);
		void progress(double seconds);
	};
}

#endif
