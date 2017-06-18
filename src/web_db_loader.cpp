#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "graph_util.hpp"
#include "metadata.hpp"
#include <iomanip>
#include <algorithm>
#include <codecvt>

using namespace GrappleMap;
using emscripten::val;

template<typename R>
emscripten::val range_tojsval(R const & v)
{
	auto a = emscripten::val::array();
	int i = 0;
	foreach (x : v) a.set(i++, x);
	return a;
}

template<typename T>
val tojsval(vector<T> const & v) { return range_tojsval(v); }

template<typename T>
val tojsval(set<T> const & s) { return range_tojsval(s); }

val tojsval(V3 const v)
{
	auto r = val::object();
	r.set("x", v.x);
	r.set("y", v.y);
	r.set("z", v.z);
	return r;
}

val tojsval(PositionReorientation const & reo)
{
	auto r = val::object();
	r.set("mirror", reo.mirror);
	r.set("swap_players", reo.swap_players);
	r.set("angle", reo.reorientation.angle);
	r.set("offset", tojsval(reo.reorientation.offset));
	return r;
}

val tojsval(ReorientedNode const & n)
{
	auto r = val::object();
	r.set("node", n->index);
	r.set("reo", tojsval(n.reorientation));
	return r;
}

val tojsval(Step const s)
{
	auto r = val::object();
	r.set("transition", s->index);
	r.set("reverse", s.reverse);
	return r;
}

val tojsval(Position const & p)
{
	vector<val> pl;

	foreach (n : playerNums())
	{
		vector<val> v;

		foreach (j : joints)
			v.push_back(tojsval(p[n][j]));

		pl.push_back(tojsval(v));
	}

	return tojsval(pl);
}

val to_elaborate_jsval(SeqNum const s, Graph const & graph)
{
	Graph::Edge const & edge = graph[s];

	vector<val> frames;
	foreach (pos : edge.positions) frames.push_back(tojsval(pos));

	auto r = val::object();
	r.set("id", s.index);
	r.set("from", tojsval(edge.from));
	r.set("to", tojsval(edge.to));
	r.set("frames", tojsval(frames));
	r.set("description", tojsval(edge.description));
	r.set("tags", tojsval(tags(edge)));
	r.set("properties", tojsval(properties_in_desc(edge.description)));
	if (edge.line_nr) r.set("line_nr", *edge.line_nr);
	return r;
}

val to_elaborate_jsval(NodeNum const n, Graph const & graph)
{
/*
	set<string> disc;
	foreach (p : query_for(graph, n))
		if (!p.second) disc.insert(p.first);
*/
	vector<val> incoming, outgoing;
	foreach (s : graph[n].in) incoming.push_back(tojsval(s));
	foreach (s : graph[n].out) outgoing.push_back(tojsval(s));

	auto node = emscripten::val::object();

	node.set("id", n.index);
	node.set("incoming", tojsval(incoming));
	node.set("outgoing", tojsval(outgoing));
	node.set("position", tojsval(graph[n].position));
	node.set("description", tojsval(graph[n].description));
	node.set("tags", tojsval(tags(graph[n])));
//	js << ",discriminators:";
//	tojs(disc, js);
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

EMSCRIPTEN_BINDINGS(GrappleMap_db)
{
	emscripten::function("loadDB", +[]
	{
		return to_elaborate_jsval(loadGraph("GrappleMap.txt"));
	});
}
