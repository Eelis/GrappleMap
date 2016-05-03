#ifndef GRAPPLEMAP_IMAGES_HPP
#define GRAPPLEMAP_IMAGES_HPP

#include "graph.hpp"
#include "headings.hpp"
#include "rendering.hpp"
#include <GL/osmesa.h>

namespace GrappleMap {

class ImageMaker
{
	Graph const & graph;

	OSMesaContext ctx = nullptr;

	void png(
		string output_dir,
		Position pos,
		double angle,
		string filename,
		unsigned width, unsigned height, V3 bg_color) const;

public:

	ImageMaker(Graph const &);
	~ImageMaker();

	ImageMaker(ImageMaker const &) = delete;
	ImageMaker & operator=(ImageMaker const &) = delete;

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

	void png(
		Position,
		Camera const &,
		unsigned width, unsigned height,
		string path, V3 bg_color,
		View, unsigned grid_size = 2, unsigned grid_line_width = 2) const;

	string png(
		string output_dir, Position, ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname = "") const;

	string rotation_gif(
		string output_dir, Position, ImageView,
		unsigned width, unsigned height, BgColor) const;

	string gif(
		string output_dir,
		vector<Position> const & frames,
		ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname) const;

	string gifs(
		string output_dir,
		vector<Position> const & frames,
		unsigned width, unsigned height, BgColor) const;
};

}

#endif
