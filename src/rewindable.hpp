#ifndef REWINDABLE_HPP
#define REWINDABLE_HPP

#include <vector>
#include <functional>

template<typename T, typename I>
auto & follow(T & x, I const & i) { return x[i]; }

template<typename T, typename M, typename U>
auto & follow(T & x, U M::* mem) { return x.*mem; }

namespace rewindable_detail
{
	template<size_t...> struct index_sequence {};

	template<size_t... I>
	inline auto extend(index_sequence<I...>) { return index_sequence<I..., sizeof...(I)>{}; }

	template<size_t I>
	inline auto make_index_sequence() { return extend(make_index_sequence<I - 1>()); }

	template<>
	inline auto make_index_sequence<1>() { return index_sequence<0>{}; }

	// todo: remove the above asap

	template<typename T>
	static T & resolve(T & x) { return x; }

	template<typename T, typename P, typename... Path>
	static auto & resolve(T & x, P p, Path ... pp) { return resolve(follow(x, p), pp...); }

	template<typename T, typename... Path, size_t... I>
	static auto & resolve(T & x, std::tuple<Path...> const & pt, index_sequence<I...>)
	{
		return resolve(x, std::get<I>(pt)...);
	}

	template<typename T, typename... Path>
	static auto & resolve(T & x, std::tuple<Path...> const & pt)
	{
		return resolve(x, pt, make_index_sequence<sizeof...(Path)>());
			// todo: use std::apply
	}
}

template<typename C>
class Rewindable
{
	C c;
	std::vector<std::vector<std::function<void()>>> ff;

	void add(std::function<void()> f)
	{
		if (ff.empty()) ff.emplace_back();
		ff.back().push_back(std::move(f));
	}

public:

	// read:

	C const * operator->() const { return &c; }

	// write:

	template<typename... Path>
	class OnPath
	{
		Rewindable & re;
		std::tuple<Path...> p;

		auto & resolve() const { return rewindable_detail::resolve(re.c, p); }

	public:

		OnPath(Rewindable & re_, std::tuple<Path...> p_)
			: re(re_), p(p_)
		{}

		auto const & operator*() const { return rewindable_detail::resolve(re.c, p); }
		auto const * operator->() const { return &**this; }

		template<typename T>
		OnPath<Path..., T> operator[](T m)
		{
			return OnPath<Path..., T>(re, std::tuple_cat(p, std::make_tuple(m)));
		}

		void erase(size_t i)
		{
			auto & x = resolve();
			auto old = std::move(x[i]);
			auto self = *this;

			re.add([self, old, i]() mutable // todo: capture *this by value
				{
					auto & y = self.resolve();
					y.insert(y.begin() + i, std::move(old));
				});

			x.erase(x.begin() + i);
		}

		template<typename T>
		void operator=(T v)
		{
			T & x = resolve();
			T old = std::move(x);
			auto self = *this;

			re.add([old, self]() mutable { self.resolve() = std::move(old); });
			x = std::move(v);
		}

		template<typename T>
		void push_back(T v)
		{
			auto self = *this;
			re.add([self]{ self.resolve().pop_back(); });
			resolve().push_back(std::move(v));
		}

		template<typename T>
		void insert(size_t i, T v)
		{
			auto self = *this;

			re.add([self, i]{
					auto & x = self.resolve();
					x.erase(x.begin() + i);
				});
			
			auto & x = resolve();
			x.insert(x.begin() + i, std::move(v));
		}

		template<typename T>
		bool operator==(T const & x) const { return resolve() == x; }

		template<typename T>
		bool operator!=(T const & x) const { return resolve() != x; }
	};

	template<typename T>
	OnPath<T> operator[](T x) { return OnPath<T>(*this, std::make_tuple(x)); }

	void forget_past() { ff.clear(); }

	void rewind_point() { ff.push_back({}); }

	void rewind()
	{
		if (ff.empty()) return;

		for (auto i = ff.back().rbegin(); i != ff.back().rend(); ++i)
			(*i)();

		ff.pop_back();
	}
};

#endif
