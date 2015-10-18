#include "persistence.hpp"
#include "util.hpp"
#include "graph.hpp"
#include <fstream>
#include <iterator>
#include <cstring>

namespace
{
	char const base62digits[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static_assert(sizeof(base62digits) == 62 + 1, "hm");

	int fromBase62(char c)
	{
		if (c >= 'a' && c <= 'z') return c - 'a';
		if (c >= 'A' && c <= 'Z') return (c - 'A') + 26;
		if (c >= '0' && c <= '9') return (c - '0') + 52;
		
		throw std::runtime_error("not a base 62 digit: " + std::string(1, c));
	}

	Position decodePosition(std::string s)
	{
		if (s.size() != 2 * joint_count * 3 * 2)
			throw std::runtime_error("position string has incorrect size");

		auto g = [&s]
			{
				double d = double(fromBase62(s[0]) * 62 + fromBase62(s[1])) / 1000;
				s.erase(0, 2);
				return d;
			};

		Position p;

		foreach (j : playerJoints)
			p[j] = {g() - 2, g(), g() - 2};

		return p;
	}
}

std::istream & operator>>(std::istream & i, std::vector<Sequence> & v)
{
	std::string line;

	while(std::getline(i, line))
		if (line.empty() or line.front() == '#')
			continue;
		else if (line.front() == ' ')
		{
			if (v.empty()) throw std::runtime_error("file contains position without preceding description");

			while (line.front() == ' ') line.erase(0, 1);

			v.back().positions.push_back(decodePosition(line));
		}
		else
		{
			v.push_back(Sequence{line, {}});
			std::cerr << "Loading: " << line << '\n';
		}

	return i;
}

std::ostream & operator<<(std::ostream & o, Position const & p)
{
	auto g = [&o](double const d)
		{
			int const i = int(std::round(d * 1000));
			assert(i >= 0);
			assert(i < 4000);
			o << base62digits[i / 62] << base62digits[i % 62];
		};

	foreach (j : playerJoints)
	{
		g(p[j].x + 2);
		g(p[j].y);
		g(p[j].z + 2);
	}

	return o;
}

std::ostream & operator<<(std::ostream & o, Sequence const & s)
{
	o << s.description << '\n';
	foreach (p : s.positions) o << "    " << p << '\n';
	return o;
}

std::vector<Sequence> load(std::string const filename)
{
	std::vector<Sequence> r;
	std::ifstream ff(filename);

	if (!ff) throw std::runtime_error(filename + ": " + std::strerror(errno));

	ff >> r;

	size_t p = 0;
	foreach (seq : r) p += seq.positions.size();
	std::cerr << "Loaded " << p << " positions in " << r.size() << " sequences.\n";

	return r;
}

void save(Graph const & g, std::string const filename)
{
	std::ofstream f(filename);
	for (SeqNum s = 0; s != g.num_sequences(); ++s) f << g.sequence(s);
}
