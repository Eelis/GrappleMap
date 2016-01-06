#ifndef GRAPPLEMAP_HEADINGS_HPP
#define GRAPPLEMAP_HEADINGS_HPP

enum class Heading: unsigned { N, E, S, W };

struct MirroredHeading
{
	Heading heading;
	bool mirror;
};

inline vector<MirroredHeading> const headings() // todo: replace with something less silly
{
	return
		{ {Heading::N, false}, {Heading::E, false}, {Heading::S, false}, {Heading::W, false}
		, {Heading::N, true}, {Heading::E, true}, {Heading::S, true}, {Heading::W, true}
		};
}

inline Heading rotate_left(Heading const h) { return Heading((unsigned(h) + 3) % 4); }
inline Heading rotate_right(Heading const h) { return Heading((unsigned(h) + 1) % 4); }

inline MirroredHeading rotate_left(MirroredHeading const mh)
{
	return MirroredHeading{rotate_left(mh.heading), mh.mirror};
}

inline MirroredHeading rotate_right(MirroredHeading const mh)
{
	return MirroredHeading{rotate_right(mh.heading), mh.mirror};
}

inline char code(MirroredHeading const mh)
{
	switch (mh.heading)
	{
		case Heading::N: return mh.mirror ? 'N' : 'n';
		case Heading::E: return mh.mirror ? 'E' : 'e';
		case Heading::S: return mh.mirror ? 'S' : 's';
		case Heading::W: return mh.mirror ? 'W' : 'w';
	}

	abort();
}

inline MirroredHeading mirror(MirroredHeading const mh)
{
	return MirroredHeading{mh.heading, !mh.mirror};
}

inline double angle(Heading const h)
{
	return M_PI * 0.5 * unsigned(h);
}

#endif
