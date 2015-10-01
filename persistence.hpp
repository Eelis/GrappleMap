#ifndef JIUJITSUMAPPER_PERSISTENCE_HPP
#define JIUJITSUMAPPER_PERSISTENCE_HPP

#include "positions.hpp"
#include <fstream>
#include <iterator>

char const base62digits[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static_assert(sizeof(base62digits) == 62 + 1, "hm");

inline int fromBase62(char c)
{
	if (c >= 'a' && c <= 'z') return c - 'a';
	if (c >= 'A' && c <= 'Z') return (c - 'A') + 26;
	if (c >= '0' && c <= '9') return (c - '0') + 52;
	
	throw std::runtime_error("not a base 62 digit: " + std::string(1, c));
}

inline Position decodePosition(std::string s)
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

	for (auto j : playerJoints)
		p[j] = {g() - 2, g(), g() - 2};

	return p;
}

inline std::istream & operator>>(std::istream & i, std::vector<Sequence> & v)
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
			v.push_back(Sequence{line});
			std::cout << "Loading: " << line << '\n';
		}

	return i;
}

inline std::ostream & operator<<(std::ostream & o, Position const & p)
{
	auto g = [&o](double const d)
		{
			int const i = int(std::round(d * 1000));
			assert(i >= 0);
			assert(i < 4000);
			o << base62digits[i / 62] << base62digits[i % 62];
		};

	for (auto j : playerJoints)
	{
		g(p[j].x + 2);
		g(p[j].y);
		g(p[j].z + 2);
	}

	return o;
}

inline std::ostream & operator<<(std::ostream & o, Sequence const & s)
{
	o << s.description << '\n';
	for (auto && p : s.positions) o << "    " << p << '\n';
	return o;
}

inline std::vector<Sequence> load(std::string const filename)
{
	std::vector<Sequence> r;
	std::ifstream ff(filename);
	ff >> r;
	return r;
}

inline void save(std::vector<Sequence> const & sequences, std::string const filename)
{
	std::ofstream f(filename);
	std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<Sequence>(f));
}

#endif
