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
}

#endif
