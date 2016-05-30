#include "util.hpp"
#include "persistence.hpp"
#include "positions.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>

int main()
{
	try
	{
		std::ofstream js("transitions.js");
		js << std::boolalpha;
		tojs(GrappleMap::loadGraph("GrappleMap.txt"), js);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
