#include "js_conversions.hpp"
#include "metadata.hpp"

namespace GrappleMap
{
	#ifdef EMSCRIPTEN

	using emscripten::val;

	val tojsval(Step const s)
	{
		auto r = val::object();
		r.set("transition", s->index);
		r.set("reverse", s.reverse);
		return r;
	}

	val to_elaborate_jsval(SeqNum const s, Graph const & graph)
	{
		Graph::Edge const & edge = graph[s];

		auto r = val::object();
		r.set("id", s.index);
		r.set("from", edge.from->index);
		r.set("to", edge.to->index);
		r.set("description", tojsval(edge.description));
		r.set("tags", tojsval(tags(edge)));
		r.set("properties", tojsval(properties_in_desc(edge.description)));
		if (edge.line_nr) r.set("line_nr", *edge.line_nr);
		return r;
	}

	val to_elaborate_jsval(NodeNum const n, Graph const & graph)
	{
		vector<val> incoming, outgoing;
		foreach (s : graph[n].in) incoming.push_back(tojsval(s));
		foreach (s : graph[n].out) outgoing.push_back(tojsval(s));

		auto node = emscripten::val::object();

		node.set("id", n.index);
		node.set("incoming", tojsval(incoming));
		node.set("outgoing", tojsval(outgoing));
		node.set("description", tojsval(graph[n].description));
		node.set("tags", tojsval(tags(graph[n])));
		if (graph[n].line_nr) node.set("line_nr", *graph[n].line_nr);

		return node;
	}

	val to_elaborate_jsval(Graph const & g)
	{
		vector<val> nodes, transitions;

		foreach (n : nodenums(g))
			nodes.push_back(to_elaborate_jsval(n, g));
		foreach (s : seqnums(g))
			transitions.push_back(to_elaborate_jsval(s, g));

		auto db = emscripten::val::object();
		db.set("nodes", tojsval(nodes));
		db.set("transitions", tojsval(transitions));
		db.set("tags", tojsval(tags(g)));

		return db;
	}

	val tojsval(NodeNum const n, Graph const & g)
	{
		auto v = emscripten::val::object();
		v.set("node", n.index);
		v.set("description", GrappleMap::tojsval(g[n].description));
		return v;
	}

	val tojsval(SeqNum const sn, Graph const & g)
	{
		Graph::Edge const & edge = g[sn];

		auto v = emscripten::val::object();
		v.set("id", sn.index);
		v.set("from", GrappleMap::tojsval(*edge.from, g));
		v.set("to", GrappleMap::tojsval(*edge.to, g));
		v.set("frames", edge.positions.size());
		v.set("description", GrappleMap::tojsval(edge.description));
		if (edge.line_nr) v.set("line_nr", *edge.line_nr);
		return v;
	}

	#endif

	void tojs(V3 const v, std::ostream & js)
	{
		js << "v3(" << v.x << "," << v.y << "," << v.z << ")";
	}

	void tojs(PositionReorientation const & reo, std::ostream & js)
	{
		js
			<< "{mirror:" << reo.mirror
			<< ",swap_players:" << reo.swap_players
			<< ",angle:" << reo.reorientation.angle
			<< ",offset:";
			
		tojs(reo.reorientation.offset, js);

		js << "}\n";
	}

	void tojs(Position const & p, std::ostream & js)
	{
		js << '[';

		foreach (n : playerNums())
		{
			js << '[';

			foreach (j : joints)
			{
				tojs(p[n][j], js);
				js << ',';
			}

			js << "],";
		}

		js << ']';
	}

	void tojs(Step const s, std::ostream & js)
	{
		js << "{transition:" << s->index << ",reverse:" << s.reverse << '}';
	}

	void tojs(ReorientedNode const & n, std::ostream & js)
	{
		js << "{node:" << n->index;
		js << ",reo:";
		tojs(n.reorientation, js);
		js << '}';
	}

	void tojs(vector<string> const & v, std::ostream & js)
	{
		js << '[';
		foreach(s : v)
		{
			js << '\'' << replace_all(replace_all(s, "\\", "\\\\"), "'", "\\'") << "',";
		}
		js << ']';
	}

	void tojs(set<string> const & v, std::ostream & js)
	{
		js << '[';
		bool first = true;
		foreach(s : v)
		{
			if (first) first = false; else js << ',';
			js << '\'' << replace_all(s, "'", "\\'") << '\'';
		}
		js << ']';
	}

	void tojs(SeqNum const s, Graph const & graph, std::ostream & js)
	{
		Graph::Edge const & edge = graph[s];

		js << "{id:" << s.index;
		js << ",from:"; tojs(edge.from, js);
		js << ",to:"; tojs(edge.to, js);
		js << ",frames:[";
		foreach (pos : edge.positions)
		{
			tojs(pos, js);
			js << ',';
		}
		js << "],description:";
		tojs(edge.description, js);
		js << ",tags:";
		tojs(tags(edge), js);
		js << ",properties:";
		tojs(properties_in_desc(edge.description), js);
		if (edge.line_nr)
			js << ",line_nr:" << *edge.line_nr;
		js << "}";
	}

	string desc(Graph::Node const & n) // TODO: bad, tojs should not alter description strings
	{
		auto desc = n.description;
		return desc.empty() ? "?" : desc.front();
	}

	void tojs(Graph const & graph, std::ostream & js)
	{
		js << "nodes=[";
		foreach (n : nodenums(graph))
		{
			set<string> disc;
			foreach (p : query_for(graph, n))
				if (!p.second) disc.insert(p.first);

			js << "{id:" << n.index << ",incoming:[";
			foreach (s : graph[n].in) { tojs(s, js); js << ','; }
			js << "],outgoing:[";
			foreach (s : graph[n].out) { tojs(s, js); js << ','; }
			js << "],position:";
			tojs(graph[n].position, js);
			js << ",description:'" << replace_all(desc(graph[n]), "'", "\\'") << "'";
			js << ",tags:";
			tojs(tags(graph[n]), js);
			js << ",discriminators:";
			tojs(disc, js);
			if (graph[n].line_nr)
				js << ",line_nr:" << *graph[n].line_nr;
			js << "},\n";
		}
		js << "];\n\n";

		js << "transitions=[";
		foreach (s : seqnums(graph))
		{
			tojs(s, graph, js);
			js << ",\n";
		}
		js << "];\n\n";

		js << "tags=";
		tojs(tags(graph), js);
		js << ";\n\n";
	}
}
