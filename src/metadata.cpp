#include "metadata.hpp"

namespace GrappleMap
{
	TagQuery query_for(Graph const & g, NodeNum const n)
	{
		TagQuery q;

		foreach (t : tags(g[n]))
			q.insert(make_pair(t, true));

		while (q.size() < 10)
		{
			map<string, int> c;

			foreach (m : match(g, q))
				if (m != n)
					foreach (t : tags(g[m]))
						if (q.find(make_pair(t, true)) == q.end()
							&& q.find(make_pair(t, false)) == q.end())
							++c[t];

			if (c.empty()) break;

			auto me = std::max_element(c.begin(), c.end(),
				[](pair<string, int> const & x, pair<string, int> const & y)
				{
					return x.second < y.second;
				});

			q.insert(make_pair(me->first, false));
		}

		return q;
	}

	set<string> properties_in_desc(vector<string> const & desc)
	{
		set<string> r;

		string const decl = "properties:";

		foreach(line : desc)
		{
			if (line.substr(0, decl.size()) == decl)
			{
				std::istringstream iss(line.substr(decl.size()));
				string prop;
				while (iss >> prop) r.insert(prop);
			}
		}

		return r;
	}

	set<string> tags(Graph const & g)
	{
		set<string> r;

		foreach(n : nodenums(g)) foreach(t : tags(g[n])) r.insert(t);
		foreach(s : seqnums(g)) foreach(t : tags(g[s])) r.insert(t);

		return r;
	}

	optional<Step> step_by_desc(Graph const & g, string const & desc, optional<NodeNum> const from)
	{
		foreach(sn : seqnums(g))
			if (replace_all(g[sn].description.front(), "\\n", " ") == desc
				|| desc == "t" + std::to_string(sn.index))
			{
				if (!from || *g[sn].from == *from)
					return Step{sn, false};
				if (g[sn].bidirectional && *g[sn].to == *from)
					return Step{sn, true};
			}

		return none;
	}

	optional<NodeNum> node_by_desc(Graph const & g, string const & desc)
	{
		if (desc.size() >= 2 && desc.front() == 'p' && all_digits(desc.substr(1)))
			return NodeNum{uint16_t(std::stoul(desc.substr(1)))};

		foreach(n : nodenums(g))
		{
			auto const & d = g[n].description;
			if (!d.empty() && replace_all(d.front(), "\\n", " ") == desc)
				return n;
		}

		return none;
	}

	optional<NamedEntity> named_entity(Graph const & g, string const & s)
	{
		if (s == "last-trans") return NamedEntity(nonreversed(SeqNum{g.num_sequences() - 1u}));

		if (auto step = step_by_desc(g, s)) return NamedEntity{*step};

		if (auto n = node_by_desc(g, s)) return NamedEntity{*n};

		return none;
	}
}
