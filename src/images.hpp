#ifndef GRAPPLEMAP_IMAGES_HPP
#define GRAPPLEMAP_IMAGES_HPP

#include "graph.hpp"
#include "headings.hpp"
#include "rendering.hpp"
#include <GL/osmesa.h>
#include <Magick++.h>
#include <unordered_set>
#include <boost/filesystem.hpp>
#include <gvc.h>

namespace GrappleMap {

namespace fs = boost::filesystem;

struct OSMesaContextPtr
{
	OSMesaContext context = nullptr;

	explicit OSMesaContextPtr(OSMesaContext c): context(c) {}

	~OSMesaContextPtr()
	{
		if (context) OSMesaDestroyContext(context);
	}
};

class ImageMaker
{
	Graph const & graph;
	OSMesaContextPtr ctx;
	GVC_t *gvc = gvContext();
	std::unordered_set<string> stored_initially, linked_initially;

	void png(
		Position pos, double angle, double ymax, string filename,
		unsigned width, unsigned height, V3 bg_color);

	vector<Magick::Image> make_frames(
		vector<pair<Position, Camera>> const &,
		size_t delay,
		unsigned width, unsigned height,
		V3 bg_color,
		View const &);

	void png(
		pair<Position, Camera> const * pos_beg,
		pair<Position, Camera> const * pos_end,
		unsigned width, unsigned height,
		string path, V3 bg_color,
		vector<View> const &, unsigned grid_size = 2, unsigned grid_line_width = 2);

	void png(
		Position,
		Camera const &,
		unsigned width, unsigned height,
		string path, V3 bg_color,
		vector<View> const &, unsigned grid_size = 2, unsigned grid_line_width = 2);

public:

	void store(
			string const & filename,
			string const & linkname,
			std::function<void()> make_dst);

	string const res_dir;

	bool no_anim = false;

	explicit ImageMaker(Graph const &, string res_dir /* e.g. path/to/GrappleMap/res */);

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
		Position, double ymax, ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string rotation_gif(
		Position, ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string gif(
		vector<Position> const & frames,
		ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string gifs(
		vector<Position> const & frames,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	void make_mp4(
		string filename, string linkname,
		size_t delay,
		unsigned width, unsigned height,
		V3 bg_color,
		View const &,
		function<vector<pair<Position, Camera>>()> make_pcs);

	void make_svg(string const & filename, string const & dot) const;
};

}

#endif
