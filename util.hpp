#ifndef JIUJITSUMAPPER_UTIL_HPP
#define JIUJITSUMAPPER_UTIL_HPP

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

#endif
