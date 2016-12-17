#include "paths.hpp"

namespace GrappleMap {

using Frames = vector<pair<string, vector<Position>>>;

Frames smoothen(Frames f)
{
	Position last_pos = f[0].second[0];

	foreach (x : f)
	foreach (p : x.second)
	foreach (j : playerJoints)
	{
		double const lag = std::min(0.83, 0.6 + p[j].y);
		p[j] = last_pos[j] = last_pos[j] * lag + p[j] * (1 - lag);
	}

	return f;
}

Frames frames(Graph const & g, Path const & path, unsigned const frames_per_pos)
{
	if (path.empty()) return Frames();

	auto d = [&](SeqNum seq)
		{
			assert(!g[seq].description.empty());
			string desc = g[seq].description.front();
			if (desc == "..." && !g[*g.to(seq)].description.empty()) desc = g[*g.to(seq)].description.front();
			desc = replace_all(desc, "\\n", " ");
			return desc;
		};

	Frames r;
	ReorientedNode n = from(g, path.front());

	foreach (step : path)
	{
		pair<vector<Position>, ReorientedNode> p =
			follow(g, n, *step, frames_per_pos / (g[*step].detailed ? 2 : 1));

		p.first.pop_back();

		r.emplace_back(d(*step), p.first);

		n = p.second;
	}

	return r;
}

Frames frames(Graph const & g, vector<Path> const & script, unsigned const frames_per_pos)
{
	Frames full;
	foreach (path : script) append(full, frames(g, path, frames_per_pos));
	return full;
}

class PathFinder
{
	Graph const & graph;
	Path scene;
	size_t unique_steps_taken = 0;
	vector<unsigned> in_seq_counts = vector<unsigned>(graph.num_sequences(), 0);
	vector<unsigned> out_seq_counts = vector<unsigned>(graph.num_sequences(), 0);
	vector<bool> standing;
	size_t longest_ever = 0;

	static constexpr SeqNum begin_trans{838};

	bool do_find(ReorientedNode const n, size_t const size)
	{
		if (size == 0) return true;

		if (double(unique_steps_taken) / scene.size() < 0.96)
			return false;

		if (graph[*n].out.size() > 32) abort();

		std::array<std::pair<size_t, Step>, 32> choices;

		auto * choices_end = choices.begin();

		foreach (s : graph[*n].out)
		{
			if (std::find(scene.end() - std::min(scene.size(), Path::size_type(15ul)), scene.end(), s) != scene.end()) continue;

			if (!scene.empty() && *from(graph, scene.back()) == *to(graph, s)) continue;

			*(choices_end++) = std::make_pair(
				(s.reverse ? in_seq_counts : out_seq_counts)[s->index] * 1000 + (rand()%1000),
				s);

			//norm2(follow(g, n, s.seq).reorientation.reorientation.offset);
		}

		std::sort(choices.begin(), choices_end);

		for (auto i = choices.begin(); i != choices_end; ++i)
		{
			Step const s = i->second;

			auto & c = (s.reverse ? in_seq_counts : out_seq_counts)[s->index];

			bool const taken_before = (c != 0);

			bool const count_as_unique = !taken_before || *s == begin_trans;

			if (count_as_unique) ++unique_steps_taken;
			++c;
			scene.push_back(s);

			if (scene.size() > longest_ever)
			{
				longest_ever = scene.size();
				//std::cout << longest_ever << std::endl;
			}

			if (do_find(follow(graph, n, *s), size - 1))
				return true;

			if (count_as_unique) --unique_steps_taken;
			--c;
			scene.pop_back();
		}

		return false;
	}

public:

	explicit PathFinder(Graph const & g)
		: graph(g)
	{
		foreach(n : nodenums(g))
			standing.push_back(tags(g[n]).count("standing") != 0);
	}

	Path find(ReorientedNode const n, size_t const size)
	{
		if (do_find(n, size))
			return scene;
		else
			throw std::runtime_error("could not find path");
	}
};

constexpr SeqNum PathFinder::begin_trans;

/*
Scene randomScene(Graph const & g, SeqNum const start, size_t const size)
{
	Scene v{start};

	auto const m = nodes(g);

	std::deque<NodeNum> recent;

	NodeNum node = g.to(start).node;

	while (v.size() < size)
	{
		vector<pair<SeqNum, NodeNum>> choices;

		foreach (s : m.at(node).second)
		{
			auto const to = g.to(s).node;
			if (std::find(recent.begin(), recent.end(), to) == recent.end() || m.at(node).first.empty())
				choices.push_back({s, g.to(s).node});
		}

		if (choices.empty())
			foreach (s : m.at(node).first)
				choices.push_back({s, g.from(s).node});

		assert(!choices.empty());

		auto p = choices[rand()%choices.size()];

		v.push_back(p.first);
		recent.push_back(p.second);
		if (recent.size() > 10) recent.pop_front();

		node = p.second;
	}

	return v;
}
*/

Path randomScene(Graph const & g, NodeNum const start, size_t const size)
{
	Path const s = PathFinder(g).find({start, {}}, size);

	int worst_count = 0;
	SeqNum worst_seq;
	map<SeqNum, int> ss;
	foreach (x : s)
	{
		auto const c = ++ss[*x];
		if (c > worst_count)
		{
			worst_count = c;
			worst_seq = *x;
		}
	}
	std::cout << ss.size() << " unique sequences\n";

	if (!ss.empty())
	{
		std::cout << "worst: " << worst_seq.index << " occurs " << worst_count << " times\n";
	}

	return s;
}

/*
Scene randomScene(Graph const & g, NodeNum const start, size_t const size)
{
	auto o = out(g, start);
	if (o.empty()) throw runtime_error("cannot start at node without outgoing nodes");

	std::random_shuffle(o.begin(), o.end());
	return randomScene(g, o.front(), size);
}
*/

vector<Path> paths_through(Graph const & g, Step s, unsigned in_size, unsigned out_size)
{
	vector<Path> v;

	foreach (pre : in_paths(g, *from(g, s), in_size))
	foreach (post : out_paths(g, *to(g, s), out_size))
	{
		Path path = pre;
		path.push_back(s);
		append(path, post);
		v.push_back(path);
	}

	return v;
}

Frames demoFrames(Graph const & g, Step const s, unsigned const frames_per_pos)
{
	Frames f;

	auto scenes = paths_through(g, s, 1, 4);

	cout << "Generating " << scenes.size() << " demo scenes.\n";

	foreach (scene : scenes)
	{
		Frames z = frames(g, scene, frames_per_pos);

		Position const last = z.back().second.back();
		z.back().second.insert(z.back().second.end(), 70, last);

		Frames const x = smoothen(z);
		f.push_back({"      ", vector<Position>(70, x.front().second.front())});
		append(f, x);
	}

	return f;
}

vector<Path> in_paths(Graph const & g, NodeNum const node, unsigned size)
{
	if (size == 0) return {Path()};

	auto const & is = g[node].in;

	if (is.empty()) return {Path()};

	vector<Path> r;

	foreach (x : is)
		foreach (e : in_paths(g, *from(g, x), size - 1))
		{
			e.push_back(x);
			r.push_back(e);
		}

	return r;
}

vector<Path> out_paths(Graph const & g, NodeNum const node, unsigned size)
{
	if (size == 0) return {Path()};

	auto const & os = g[node].out;

	if (os.empty()) return {Path()};

	vector<Path> r;

	foreach (x : os)
		foreach (e : out_paths(g, *to(g, x), size - 1))
		{
			e.insert(e.begin(), x);
			r.push_back(e);
		}

	return r;
}

void reorient_from(
	OrientedPath & path,
	OrientedPath::iterator const start,
	Graph const & g)
{
	Reoriented<NodeNum> n = from(*start, g);

	for (auto i = start; i != path.begin(); --i)
	{
		auto & x = *prev(i);
		x = gp_connect(n, *x, g);
		n = from(x, g);
	}

	n = to(*start, g);

	for (auto i = next(start); i != path.end(); ++i)
	{
		*i = gp_connect(n, **i, g);
		n = to(*i, g);
	}
}

optional<Reoriented<Location>> advance_along(
	Reoriented<Location> location, OrientedPath const & path, Graph const & graph)
{
	auto i = path.begin();

	for (; i != path.end(); ++i)
	{
		Reoriented<Reversible<SeqNum>> const & seq = *i;

		if (**seq == location->segment.sequence)
		{
			if (seq->reverse)
			{
				if (location->howFar > 0)
				{
					location->howFar = 0;
					return location;
				}

				if (location->segment.segment != SegmentNum{0})
				{
					--location->segment.segment;
					return location;
				}
			}
			else
			{
				if (location->howFar < 1)
				{
					location->howFar = 1;
					return location;
				}

				if (location->segment != last_segment(location->segment.sequence, graph))
				{
					++location->segment.segment;
					return location;
				}
			}

			if (++i == path.end()) return boost::none;

			return to_loc(first_segment(*i, graph));
		}
	}

	return boost::none;
}

optional<Reoriented<Location>> retreat_along(
	Reoriented<Location> location, OrientedPath const & path, Graph const & graph)
{
	auto i = path.rbegin();

	for (; i != path.rend(); ++i)
	{
		Reoriented<Reversible<SeqNum>> const & seq = *i;

		if (**seq == location->segment.sequence)
		{
			if (seq->reverse)
			{
				if (location->howFar < 1)
				{
					location->howFar = 1;
					return location;
				}

				if (location->segment != last_segment(location->segment.sequence, graph))
				{
					++location->segment.segment;
					return location;
				}
			}
			else
			{
				if (location->howFar > 0)
				{
					location->howFar = 0;
					return location;
				}

				if (location->segment.segment != SegmentNum{0})
				{
					--location->segment.segment;
					return location;
				}
			}

			if (++i == path.rend()) return boost::none;

			return from_loc(last_segment(*i, graph));
		}
	}

	return boost::none;
}

}
