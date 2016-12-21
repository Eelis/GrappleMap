#ifndef GRAPPLEMAP_METADATA_HPP
#define GRAPPLEMAP_METADATA_HPP

#include "graph_util.hpp"
#include <boost/variant.hpp>

namespace GrappleMap
{
	using NamedEntity = boost::variant<NodeNum, Step>;

	optional<Step> step_by_desc(Graph const &, string const & desc, optional<NodeNum> from = none);
	optional<NodeNum> node_by_desc(Graph const &, string const & desc);
	optional<NamedEntity> named_entity(Graph const & g, string const & desc);

	set<string> tags(Graph const &);
	set<string> tags_in_desc(vector<string> const &);
	set<string> properties_in_desc(vector<string> const &);

	inline set<string> tags(Graph::Node const & n) { return tags_in_desc(n.description); }
	inline set<string> tags(Sequence const & s) { return tags_in_desc(s.description); }

	inline set<string> properties(Sequence const & s)
	{
		return properties_in_desc(s.description);
	}

	inline bool is_tagged(Graph const & g, string const & tag, NodeNum const n)
	{
		return elem(tag, tags(g[n]));
	}

	inline bool is_tagged(Graph const & g, string const & tag, SeqNum const sn)
	{
		return elem(tag, tags(g[sn]))
			|| (is_tagged(g, tag, *g[sn].from)
			&& is_tagged(g, tag, *g[sn].to));
	}

	inline bool has_property(string const & p, Sequence const & s)
	{
		return elem(p, properties(s));
	}

	inline bool is_top_move(Sequence const & s) { return has_property("top", s); }
	inline bool is_bottom_move(Sequence const & s) { return has_property("bottom", s); }

	inline auto tagged_nodes(Graph const & g, string const & tag)
	{
		return nodenums(g) | filtered(
			[&g, tag](NodeNum n){ return is_tagged(g, tag, n); });
	}

	inline auto tagged_sequences(Graph const & g, string const & tag)
	{
		return seqnums(g) | filtered(
			[&g, tag](SeqNum n){ return is_tagged(g, tag, n); });
	}

	using TagQuery = set<pair<string /* tag */, bool /* include/exclude */>>;

	inline auto match(Graph const & g, TagQuery const & q)
	{
		return nodenums(g) | filtered(
			[&g, q](NodeNum const n)
			{
				foreach (e : q)
					if (is_tagged(g, e.first, n) != e.second)
						return false;
				return true;
			});
	}

	TagQuery query_for(Graph const &, NodeNum);
}

#endif
