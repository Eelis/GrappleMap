#ifndef GRAPPLEMAP_PATHS_HPP
#define GRAPPLEMAP_PATHS_HPP

#include "graph_util.hpp"

namespace GrappleMap
{
	using Path = std::deque<Reversible<SeqNum>>;
	using Frames = vector<pair<string /* caption */, vector<Position>>>;
	using OrientedPath = std::deque<Reoriented<Reversible<SeqNum>>>;

	template<typename T>
	inline bool elem(T const & n, std::deque<Reversible<T>> const & d)
	{
		return std::any_of(d.begin(), d.end(),
			[&](Reversible<T> const & x)
			{
				return *x == n;
			});
	}

	inline bool elem(SeqNum const & n, OrientedPath const & s)
	{
		return std::any_of(s.begin(), s.end(),
			[&](Reoriented<Reversible<SeqNum>> const & x)
			{
				return **x == n;
			});
	}

	Frames smoothen(Frames);

	void reorient_from(OrientedPath &, OrientedPath::iterator, Graph const &);

	Frames frames(Graph const &, Path const &, unsigned frames_per_pos);

	Frames frames(Graph const &, vector<Path> const & script, unsigned frames_per_pos);

	Path randomScene(Graph const &, NodeNum start, size_t);

	vector<Path> paths_through(Graph const &, Step, unsigned in_size, unsigned out_size);

	Frames demoFrames(Graph const &, Step, unsigned frames_per_pos);

	inline vector<Position> no_titles(Frames const & f)
	{
		vector<Position> r;
		foreach (x : f) r += x.second;
		return r;
	}

	vector<Path> in_paths(Graph const &, NodeNum, unsigned size);
	vector<Path> out_paths(Graph const &, NodeNum, unsigned size);
}

#endif
