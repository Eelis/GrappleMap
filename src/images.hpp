#ifndef GRAPPLEMAP_IMAGES_HPP
#define GRAPPLEMAP_IMAGES_HPP

#include "graph.hpp"
#include "headings.hpp"
#include <GLFW/glfw3.h>

struct ImageMaker
{
	GLFWwindow * const window;
	Graph const & graph;

	enum BgColor { RedBg, BlueBg, WhiteBg };

	static V3 color(BgColor const c)
	{
		switch (c)
		{
			case RedBg: return V3{1,.878,.878};
			case BlueBg: return V3{.878,.878,1};
			case WhiteBg: return V3{1,1,1};
			default: abort();
		}
	}

	static string css(BgColor const c)
	{
		switch (c)
		{
			case RedBg: return "background:#ffe0e0";
			case BlueBg: return "background:#e0e0ff";
			case WhiteBg: return "";
			default: abort();
		}
	}

	string png(
		string const output_dir,
		Position const pos,
		MirroredHeading,
		unsigned const width, unsigned const height, BgColor const bg_color) const;

	string rotation_gif(
		string const output_dir, Position const p,
		unsigned const width, unsigned const height, BgColor const bg_color) const;

	string gif(
		string const output_dir,
		vector<Position> const & frames,
		MirroredHeading,
		unsigned const width, unsigned const height, BgColor const bg_color) const;

	string gifs(
		string const output_dir,
		vector<Position> const & frames,
		unsigned const width, unsigned const height, BgColor bg_color) const;

private:

	void png(
		string output_dir,
		Position pos,
		double angle,
		string filename,
		unsigned width, unsigned height, V3 bg_color) const;
};

#endif
