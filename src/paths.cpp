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
			if (desc == "..." && !g[g.to(seq).node].description.empty()) desc = g[g.to(seq).node].description.front();
			desc = replace_all(desc, "\\n", " ");
			return desc;
		};

	Frames r;
	ReorientedNode n = from(g, path.front());

	foreach (step : path)
	{
		pair<vector<Position>, ReorientedNode> p = follow(g, n, step.seq, frames_per_pos);

		p.first.pop_back();

		r.emplace_back(d(step.seq), p.first);

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

bool dfsScene(
	Graph const & g,
	vector<pair<vector<Step>, vector<Step>>> const & in_out,
	ReorientedNode const n, size_t const size, Path & scene, vector<Step> & steps_taken)
{
	if (size == 0) return true;

	if (double(steps_taken.size()) / scene.size() < 0.93)
		return false;

	std::multimap<std::pair<size_t /* occurrences */, double>, Step> choices;

	foreach (s : in_out[n.node.index].second)
	{
		if (std::find(scene.end() - std::min(scene.size(), Path::size_type(15ul)), scene.end(), s) != scene.end()) continue;

		size_t const c = std::count(scene.begin(), scene.end(), s);

		if (!scene.empty() && scene.back().seq == s.seq) continue;

		double const score = rand()%1000;//norm2(follow(g, n, s.seq).reorientation.reorientation.offset);

		choices.insert({{c, score}, s});
	}

	foreach (c : choices)
	{
		Step const s = c.second;

		bool const taken_before =
			std::find(steps_taken.begin(), steps_taken.end(), s) != steps_taken.end();

		if (!taken_before) steps_taken.push_back(s);
		scene.push_back(s);

		if (dfsScene(g, in_out, follow(g, n, s.seq), size - 1, scene, steps_taken))
			return true;

		if (!taken_before) steps_taken.pop_back();
		scene.pop_back();

	}

	return false;
}

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
	Path s;
	vector<Step> steps_taken;

	if (!dfsScene(g, in_out(g), {start, {}}, size, s, steps_taken))
		throw runtime_error("could not find sequence");

	int worst_count = 0;
	SeqNum worst_seq;
	map<SeqNum, int> ss;
	foreach (x : s)
	{
		auto const c = ++ss[x.seq];
		if (c > worst_count)
		{
			worst_count = c;
			worst_seq = x.seq;
		}
	}
	std::cout << ss.size() << " unique sequences\n";

	if (!ss.empty())
	{
		auto & p = *ss.rbegin();
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

	foreach (pre : in_paths(g, from(g, s).node, in_size))
	foreach (post : out_paths(g, to(g, s).node, out_size))
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

	auto scenes = paths_through(g, s, 1, 1);

	cout << "Playing " << scenes.size() << " scenes.\n";

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

}
