#ifndef GRAPPLEMAP_UTIL_HPP
#define GRAPPLEMAP_UTIL_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/range.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>

namespace GrappleMap
{
	using std::array;
	using std::string;
	using std::vector;
	using std::set;
	using std::unordered_set;
	using std::map;
	using std::istream;
	using std::ostream;
	using std::ofstream;
	using std::function;
	using std::move;
	using std::cout;
	using std::cerr;
	using std::endl;
	using std::pair;
	using std::to_string;
	using std::make_pair;
	using std::runtime_error;
	using std::swap;
	using std::exception;
	using std::unique_ptr;
	using boost::optional;
	using boost::none;
	using boost::adaptors::filtered;
	using boost::adaptors::transformed;

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

	#define foreach(...) for(auto && __VA_ARGS__)

	[[noreturn]] inline void error(string const & s) { throw runtime_error(s); }

	inline string replace_all(string s, string what, string with)
	{
		boost::algorithm::replace_all(s, what, with);
		return s;
	}

	template<typename Range>
	auto make_set(Range const & r)
	{
		using I = typename boost::range_iterator<Range>::type;
		return set<typename std::iterator_traits<I>::value_type>(begin(r), end(r));
	}

	inline bool all_digits(string const & s)
	{
		return all_of(s.begin(), s.end(), [](char c){return std::isdigit(c);});
	}

	template<typename T>
	void append(vector<T> & v, vector<T> const & w)
	{
		v.insert(v.end(), w.begin(), w.end());
	}

	template<typename T>
	void append(std::deque<T> & v, std::deque<T> const & w)
	{
		v.insert(v.end(), w.begin(), w.end());
	}

	template<typename R>
	string join(R const & rng, string const & sep)
	{
		bool first = true;
		string r;
		foreach(x : rng)
		{
			if (first) first = false; else r += sep;
			r += x;
		}
		return r;
	}

	template<typename T>
	optional<T> optionalopt(boost::program_options::variables_map const & vm, string const & name)
	{
		if (vm.count(name)) return vm[name].as<T>();
		return optional<T>();
	}

	template<typename T>
	bool elem(T const & x, vector<T> const & v)
	{
		return std::find(v.begin(), v.end(), x) != v.end();
	}

	template<typename T, typename U>
	bool elem(T const & x, set<U> const & s)
	{
		return s.find(x) != s.end();
	}

	template<typename T, typename U>
	bool elem(T const & x, unordered_set<U> const & s)
	{
		return s.find(x) != s.end();
	}

	template<typename K, typename V>
	bool elem(K const & k, map<K, V> const & m)
	{
		return m.find(k) != m.end();
	}

	template<typename T>
	vector<T> opt_as_vec(optional<T> const & o)
	{
		return o ? vector<T>{*o} : vector<T>{};
	}

	template<typename T>
	vector<T> & operator+=(vector<T> & v, vector<T> const & w)
	{
		v.reserve(v.size() + w.size());
		foreach (x : w) v.push_back(x);
		return v;
	}

	template<typename T>
	vector<T> operator+(vector<T> l, vector<T> const & r)
	{
		return l += r;
	}

	template<typename T>
	vector<T> make_vector(T x)
	{
		vector<T> r;
		r.emplace_back(std::move(x));
		return r;
	}

	inline void flip(bool & b) { b = !b; }

	inline void progress(size_t items_done, size_t items)
	{
		cout
			<< string(4, '\b')
			<< std::setw(3) << std::right << items_done * 100 / items
			<< '%' << std::flush;
	}
}

#endif
