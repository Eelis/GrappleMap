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
		Graph const & graph;
		Selection const & selection;
		Selection::const_iterator i = selection.begin();
		SegmentNum segment;
		double howFar;
		Position chaser;

		Reoriented<Location> location() const
		{
			assert (i != selection.end());

			return {
				Location{{***i, segment}, howFar},
				i->reorientation};
		}

	public:

		Playback(Graph const &, Selection const &);

		Position const & getPosition() const { return chaser; }

		void reset();
		void frame(double seconds);
		void progress(double seconds);
	};
}

#endif
