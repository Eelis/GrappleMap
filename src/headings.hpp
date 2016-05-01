#ifndef GRAPPLEMAP_HEADINGS_HPP
#define GRAPPLEMAP_HEADINGS_HPP

#include <array>
#include "positions.hpp"

namespace GrappleMap
{
	enum class Heading: uint8_t { N, E, S, W };

	struct ImageView
	{
		bool mirror;
		optional<Heading> heading;
		optional<PlayerNum> player;
	};

	inline std::array<Heading, 4> headings()
	{
		return { Heading::N, Heading::E, Heading::S, Heading::W };
	}

	inline char code(Heading const h) { return "nesw"[unsigned(h)]; }

	inline char code(ImageView const & view)
	{
		char r;
		if (view.heading) r = GrappleMap::code(*view.heading);
		else if (view.player) r = (*view.player == 0 ? 't' : 'b');
		else abort();
		return view.mirror ? std::toupper(r) : r;
	}

	inline vector<ImageView> views()
	{
		vector<ImageView> vv;

		foreach (h : headings())
		foreach (b : {true, false})
			vv.push_back(ImageView{b, h, {}});

		foreach (p : playerNums())
		foreach (b : {true, false})
			vv.push_back(ImageView{b, {}, p});

		return vv;
	}

	inline Heading rotate_left(Heading const h) { return Heading((unsigned(h) + 3) % 4); }
	inline Heading rotate_right(Heading const h) { return Heading((unsigned(h) + 1) % 4); }

	inline Heading opposite(Heading h)
	{
		switch (h)
		{
			case Heading::N: return Heading::S;
			case Heading::S: return Heading::N;
			case Heading::E: return Heading::W;
			case Heading::W: return Heading::E;
		}

		abort();
	}

	inline double angle(Heading const h)
	{
		return M_PI * 0.5 * unsigned(h);
	}
}

#endif
