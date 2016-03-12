#ifndef GRAPPLEMAP_IMAGES_HPP
#define GRAPPLEMAP_IMAGES_HPP

#include "graph.hpp"
#include "headings.hpp"
#include <GLFW/glfw3.h>
#include <GL/osmesa.h>

#define USE_OSMESA 0
	// TODO: Figure out how to make anti-aliasing work with osmesa.

class ImageMaker
{
	Graph const & graph;

	#if USE_OSMESA
		OSMesaContext ctx = nullptr;
	#else
		GLFWwindow * window = nullptr;
	#endif

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

	string png(
		string output_dir, Position, ImageView,
		unsigned width, unsigned height, BgColor) const;

	string rotation_gif(
		string output_dir, Position, ImageView,
		unsigned width, unsigned height, BgColor) const;

	string gif(
		string output_dir,
		vector<Position> const & frames,
		ImageView,
		unsigned width, unsigned height, BgColor) const;

	string gifs(
		string output_dir,
		vector<Position> const & frames,
		unsigned width, unsigned height, BgColor) const;
};

#endif
