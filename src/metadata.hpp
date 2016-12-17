#ifndef GRAPPLEMAP_METADATA_HPP
#define GRAPPLEMAP_METADATA_HPP

#include "graph_util.hpp"

namespace GrappleMap
{
	optional<Step> step_by_desc(Graph const &, string const & desc, optional<NodeNum> from = none);
	optional<NodeNum> node_by_desc(Graph const &, string const & desc);
	optional<PositionInSequence> posinseq_by_desc(Graph const & g, string const & desc);

	optional<PositionInSequence> node_as_posinseq(Graph const &, NodeNum);
		// may return either the beginning of a sequence or the end

	set<string> tags(Graph const &);
	set<string> tags_in_desc(vector<string> const &);
	set<string> properties_in_desc(vector<string> const &);

	inline set<string> tags(Graph::Node const & n)
	{
		return tags_in_desc(n.description);
	}

	inline set<string> tags(Sequence const & s)
	{
		return tags_in_desc(s.description);
	}

	inline set<string> properties(Graph const & g, SeqNum const s)
	{
		return properties_in_desc(g[s].description);
	}

	inline bool is_tagged(Graph const & g, string const & tag, NodeNum const n)
	{
		return tags(g[n]).count(tag) != 0;
	}

	inline bool is_tagged(Graph const & g, string const & tag, SeqNum const sn)
	{
		return tags(g[sn]).count(tag) != 0
			|| (is_tagged(g, tag, *g[sn].from)
			&& is_tagged(g, tag, *g[sn].to));
	}

	inline bool is_top_move(Sequence const & s)
	{
		return properties_in_desc(s.description).count("top") != 0;
	}

	inline bool is_bottom_move(Sequence const & s)
	{
		return properties_in_desc(s.description).count("bottom") != 0;
	}

	inline auto tagged_nodes(Graph const & g, string const & tag)
	{
		return nodenums(g) | boost::adaptors::filtered(
			[&](NodeNum n){ return is_tagged(g, tag, n); });
	}

	inline auto tagged_sequences(Graph const & g, string const & tag)
	{
		return seqnums(g) | boost::adaptors::filtered(
			[&](SeqNum n){ return is_tagged(g, tag, n); });
	}

	using TagQuery = set<pair<string /* tag */, bool /* include/exclude */>>;

	inline auto match(Graph const & g, TagQuery const & q)
	{
		return nodenums(g) | boost::adaptors::filtered(
			[&](NodeNum n){
				foreach (e : q)
				{
					if (is_tagged(g, e.first, n) != e.second)
						return false;
				}
				return true;
				});
	}

	TagQuery query_for(Graph const &, NodeNum);
}

#endif
