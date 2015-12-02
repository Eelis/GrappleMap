#ifndef JIUJITSUMAPPER_UTIL_HPP
#define JIUJITSUMAPPER_UTIL_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <boost/optional.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>

using std::string;
using std::vector;
using std::set;
using std::istream;
using std::ostream;
using std::move;
using std::cout;
using std::pair;
using std::to_string;
using std::make_pair;
using boost::optional;
using boost::none;

template<typename I, typename F>
I minimal(I i, I e, F f)
{
	if (i == e) return i;
	auto r = i;
	auto v = f(*i);
	for (; i != e; ++i)
	{
		auto w = f(*i);
		if (w < v)
		{
			r = i;
			v = w;
		}
	}
	return r;
}

#define foreach(x) for(auto && x)

inline void error(string const & s) { throw std::runtime_error(s); }

inline string replace_all(string s, string what, string with)
{
	boost::algorithm::replace_all(s, what, with);
	return s;
}

#endif
