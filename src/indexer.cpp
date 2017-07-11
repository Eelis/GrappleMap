#include "persistence.hpp"
#include <iomanip>
#include <iterator>

int main(int const argc, char const * const * const argv)
{
	try
	{
		if (argc != 2) return 1;
		GrappleMap::loadGraph(argv[1]);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
