#ifndef GRAPPLEMAP_JS_CONVERSIONS
#define GRAPPLEMAP_JS_CONVERSIONS

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#endif

#include "graph_util.hpp"

namespace GrappleMap
{
	#ifdef EMSCRIPTEN

	using emscripten::val;

	template<typename R>
	val range_tojsval(R const & v)
	{
		auto a = val::array();
		int i = 0;
		foreach (x : v) a.set(i++, x);
		return a;
	}

	template<typename T>
	val tojsval(vector<T> const & v) { return range_tojsval(v); }

	template<typename T>
	val tojsval(set<T> const & s) { return range_tojsval(s); }

	val tojsval(Step);
	val tojsval(NodeNum, Graph const &);
	val tojsval(SeqNum, Graph const &);

	val to_elaborate_jsval(SeqNum, Graph const &);
	val to_elaborate_jsval(NodeNum, Graph const &);
	val to_elaborate_jsval(Graph const &);

	#endif

	void tojs(vector<string> const &, std::ostream &);
	void tojs(PositionReorientation const &, std::ostream &);
	void tojs(SeqNum, Graph const &, std::ostream &);
	void tojs(Graph const &, std::ostream &);
}

#endif
