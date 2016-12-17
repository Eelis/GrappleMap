#ifndef GRAPPLEMAP_INDEXED_HPP
#define GRAPPLEMAP_INDEXED_HPP

#include "util.hpp"
#include <boost/range/counting_range.hpp>

namespace GrappleMap
{
	enum class Indexed { sequence, segment, node, position, player };

	template <Indexed, typename T>
	struct Index
	{
		using underlying_type = T;
		T index;

		using iterator = boost::counting_iterator<Index, std::forward_iterator_tag, int32_t>;
		using range = boost::iterator_range<iterator>;
	};

	template <Indexed i, typename T>
	bool operator==(Index<i, T> a, Index<i, T> b) { return a.index == b.index; }

	template <Indexed i, typename T>
	bool operator!=(Index<i, T> a, Index<i, T> b) { return a.index != b.index; }

	template <Indexed i, typename T>
	bool operator<(Index<i, T> a, Index<i, T> b) { return a.index < b.index; }

	template <Indexed i, typename T>
	Index<i, T> & operator++(Index<i, T> & x)
	{ ++x.index; return x; }

	template <Indexed i, typename T>
	Index<i, T> & operator--(Index<i, T> & x)
	{ --x.index; return x; }

	template <Indexed i, typename T>
	optional<Index<i, T>> prev(Index<i, T> x)
	{
		if (x.index == 0) return boost::none;
		return Index<i, T>{T(x.index - 1)};
	}

	template <Indexed i, typename T>
	Index<i, T> next(Index<i, T> x)
	{
		return Index<i, T>{T(x.index + 1)};
	}

	template <Indexed i, typename T>
	std::ostream & operator<<(std::ostream & o, Index<i, T> const x)
	{
		return o << int64_t(x.index);
	}

	using SeqNum = Index<Indexed::sequence, uint_fast32_t>;
	using SegmentNum = Index<Indexed::segment, uint_fast8_t>;
	using NodeNum = Index<Indexed::node, uint16_t>;
	using PosNum = Index<Indexed::position, uint_fast8_t>;
	using PlayerNum = Index<Indexed::player, uint_fast8_t>;
}

#endif
