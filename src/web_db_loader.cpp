#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include "persistence.hpp"
#include "js_conversions.hpp"

EMSCRIPTEN_BINDINGS(GrappleMap_db)
{
	emscripten::function("loadDB", +[]
	{
		return to_elaborate_jsval(GrappleMap::loadGraph("GrappleMap.txt"), false);
	});
}
