#include "png.h"
#include "images.hpp"
#include "camera.hpp"
#include "rendering.hpp"
#include <boost/program_options.hpp>
#include <unistd.h>

namespace GrappleMap
{
	inline std::size_t hash_value(V3 const v) // todo: put elsewhere
	{
		size_t seed = 0;
		boost::hash_combine(seed, v.x);
		boost::hash_combine(seed, v.y);
		boost::hash_combine(seed, v.z);
		return seed;
	}
}

#include <boost/functional/hash.hpp>

#ifndef int_p_NULL // ffs
	#define int_p_NULL (int*)NULL // https://github.com/ignf/gilviewer/issues/8
#endif

#include <boost/gil/extension/io/png_io.hpp>
#include <boost/gil/gil_all.hpp>
#include <boost/filesystem.hpp>

namespace GrappleMap {

auto hash_value(Position const & p) { return boost::hash_value(p.values); }

namespace
{
	void make_gif(
		string const output_dir,
		string const filename,
		unsigned const delay,
		std::function<vector<string>(string)> mk_frames)
	{
		if (!boost::filesystem::exists(output_dir + "/" + filename))
		{
			string const gif_frames_dir = output_dir + "/gifframes/";

			string command = "convert -depth 8 -delay " + to_string(delay) + " -loop 0 ";
			foreach (f : mk_frames(gif_frames_dir)) command += gif_frames_dir + f + ' ';
			command += output_dir + "/" + filename;

			if (std::system(command.c_str()) != 0)
				throw std::runtime_error("command failed: " + command);
		}
	}

	vector<double> smoothen_v(vector<double> const & v)
	{
		vector<double> r;

		for (size_t i = 0; i != v.size(); ++i)
		{
			double c = 1;
			double t = v[i];
			
			if (i != 0) { t += v[i-1]; ++c; }
			if (i != v.size() - 1) { t += v[i+1]; ++c; }

			r.push_back(t / c);
		}

		return r;
	}
}

void ImageMaker::png(
	Position pos,
	Camera const & camera,
	unsigned const width, unsigned const height,
	string const path, V3 const bg_color,
	vector<View> const & view,
	unsigned const grid_size, unsigned const grid_line_width)
{
	if (boost::filesystem::exists(path)) return;

	vector<boost::gil::rgb8_pixel_t> buf(width*2 * height*2);

	if (!OSMesaMakeCurrent(ctx.context, buf.data(), GL_UNSIGNED_BYTE, width*2, height*2))
		error("OSMesaMakeCurrent");

	Style style;
	style.grid_size = grid_size;
	style.grid_line_width = grid_line_width;
	style.grid_color = bg_color * .8;
	style.background_color = bg_color;

	PlayerDrawer playerDrawer;

	renderWindow(
		view,
		{}, // no viables
		graph, pos, camera,
		none, // no highlighted joint
		{}, // default colors
		0, 0, width*2, height*2,
		{},
		style, playerDrawer);

	glFlush();
	glFinish();

	foreach (p : buf) std::swap(p[0], p[2]);

	vector<boost::gil::rgb8_pixel_t> buf2(width * height);

	auto xy = [&](unsigned x, unsigned y){ return buf[y*width*2+x]; };

	for (unsigned x = 0; x != width; ++x)
	for (unsigned y = 0; y != height; ++y)
	{
		auto const & a = xy(x*2,  y*2  );
		auto const & b = xy(x*2+1,y*2  );
		auto const & c = xy(x*2,  y*2+1);
		auto const & d = xy(x*2+1,y*2+1);

		buf2[y*width+x][0] = (a[0] + b[0] + c[0] + d[0]) / 4;
		buf2[y*width+x][1] = (a[1] + b[1] + c[1] + d[1]) / 4;
		buf2[y*width+x][2] = (a[2] + b[2] + c[2] + d[2]) / 4;
	}
		// todo: clean up

	try
	{
		boost::gil::png_write_view(path,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf2.data(), width*3)));
	}
	catch (std::ios_base::failure const &)
	{
		error("could not write to " + path);
	}
}

vector<Magick::Image> ImageMaker::make_frames(
	vector<pair<Position, Camera>> const & pcs,
	size_t const delay,
	unsigned const width, unsigned const height,
	V3 const bg_color,
	View const & view)
{
	size_t const
		n = pcs.size(),
		columns = 10,
		rows = n / columns + 1,
		aawidth = width * 2,
		aaheight = height * 2,
		fullwidth = columns * aawidth,
		fullheight = rows * aaheight;

	vector<boost::gil::rgb8_pixel_t> buf(fullwidth * fullheight);

	if (!OSMesaMakeCurrent(ctx.context, buf.data(), GL_UNSIGNED_BYTE, fullwidth, fullheight))
		error("OSMesaMakeCurrent");

	Style style;
	style.grid_size = 2;
	style.grid_line_width = 2;
	style.grid_color = bg_color * .8;
	style.background_color = bg_color;

	PlayerDrawer playerDrawer;

	for (size_t i = 0; i != n; ++i)
	{
		size_t const
			row = i / columns,
			column = i % columns;

		renderBasic(
			view,
			graph, pcs[i].first, pcs[i].second,
			{}, // default colors
			column * aawidth, row * aaheight, aawidth, aaheight,
			style, playerDrawer);

		if (no_anim) break;
	}

	glFlush();
	glFinish();

	vector<Magick::Image> frames;

	for (size_t i = 0; i != n; ++i)
	{
		Magick::Image img(Magick::Geometry(width, height), "white");
		img.animationDelay(delay);
		img.modifyImage();
		Magick::Pixels pixcache(img);
		Magick::PixelPacket * pixels = pixcache.get(0, 0, width, height);

		size_t const
			row = i / columns,
			column = i % columns;

		auto xy = [&](unsigned x, unsigned y)
			{
				return buf[(row * aaheight + y) * (columns * aawidth) + (column * aawidth + x)];
			};

		for (unsigned x = 0; x != width; ++x)
		for (unsigned y = 0; y != height; ++y)
		{
			auto rgb0 = xy(x*2,y*2);
			auto rgb1 = xy(x*2,y*2+1);
			auto rgb2 = xy(x*2+1,y*2);
			auto rgb3 = xy(x*2+1,y*2+1);
			Magick::PixelPacket & pix = pixels[(height - 1 - y) * width + x];
			pix.blue  = (rgb0[0] + rgb1[0] + rgb2[0] + rgb3[0]) << (8 - 2);
			pix.green = (rgb0[1] + rgb1[1] + rgb2[1] + rgb3[1]) << (8 - 2);
			pix.red   = (rgb0[2] + rgb1[2] + rgb2[2] + rgb3[2]) << (8 - 2);
		}

		pixcache.sync();
		frames.push_back(move(img));

		if (no_anim) break;
	}

	return frames;
}

void ImageMaker::png(
	pair<Position, Camera> const * pos_b,
	pair<Position, Camera> const * pos_e,
	unsigned const width, unsigned const height,
	string const path, V3 const bg_color,
	vector<View> const & view,
	unsigned const grid_size, unsigned const grid_line_width)
{
	if (boost::filesystem::exists(path)) return;

	vector<boost::gil::rgb8_pixel_t> buf(width*2 * height*2);

	if (!OSMesaMakeCurrent(ctx.context, buf.data(), GL_UNSIGNED_BYTE, width*2, height*2))
		error("OSMesaMakeCurrent");

	Style style;
	style.grid_size = grid_size;
	style.grid_line_width = grid_line_width;
	style.grid_color = bg_color * .8;
	style.background_color = bg_color;

	glClearAccum(0.0, 0.0, 0.0, 0.0);
	glClear(GL_ACCUM_BUFFER_BIT);

	PlayerDrawer playerDrawer;

	for (pair<Position, Camera> const * p = pos_b; p != pos_e; ++p)
	{
		renderWindow(
			view,
			{}, // no viables
			graph, p->first, p->second,
			none, // no highlighted joint
			{}, // default colors
			0, 0, width*2, height*2,
			{},
			style, playerDrawer);

		glFinish();
		glAccum(GL_ACCUM, 1. / (pos_e - pos_b));
	}

	glAccum(GL_RETURN, 1.0);

	foreach (p : buf) std::swap(p[0], p[2]);

	vector<boost::gil::rgb8_pixel_t> buf2(width * height);

	auto xy = [&](unsigned x, unsigned y){ return buf[y*width*2+x]; };

	for (unsigned x = 0; x != width; ++x)
	for (unsigned y = 0; y != height; ++y)
	{
		auto const & a = xy(x*2,  y*2  );
		auto const & b = xy(x*2+1,y*2  );
		auto const & c = xy(x*2,  y*2+1);
		auto const & d = xy(x*2+1,y*2+1);

		buf2[y*width+x][0] = (a[0] + b[0] + c[0] + d[0]) / 4;
		buf2[y*width+x][1] = (a[1] + b[1] + c[1] + d[1]) / 4;
		buf2[y*width+x][2] = (a[2] + b[2] + c[2] + d[2]) / 4;
	}
		// todo: clean up

	boost::gil::png_write_view(path,
		boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf2.data(), width*3)));
}

void ImageMaker::png(
	Position const pos,
	double const angle,
	double const ymax,
	string const filename,
	unsigned const width, unsigned const height, V3 const bg_color)
{
	if (boost::filesystem::exists(filename)) return;

	Camera camera;
	camera.hardSetOffset({0, ymax - 0.57, 0});
	camera.zoom(0.55);
	camera.rotateHorizontal(angle);
	camera.rotateVertical((ymax - 0.6)/2);

	png(pos, camera, width, height, filename, bg_color, {{0, 0, 1, 1, none, 45}});
}

string ImageMaker::png(
	string const output_dir,
	Position pos,
	double const ymax,
	ImageView const view,
	unsigned const width, unsigned const height,
	BgColor const bg_color, string const base_linkname)
{
	string const attrs = code(view) + to_string(width) + 'x' + to_string(height);

	string filename =
		to_string(hash_value(pos))
		+ attrs
		+ '-' + to_string(bg_color) + ".png";

	if (!base_linkname.empty()) filename = "store/" + filename;

	if (view.mirror) pos = mirror(pos);

	if (view.heading)
		png(pos, angle(*view.heading), ymax, output_dir + "/" + filename, width, height, color(bg_color));
	else if (view.player)
	{
		Camera camera;

		png(pos, camera, width, height, output_dir + "/" + filename, color(bg_color),
			{{0, 0, 1, 1, *view.player, 80}});
	}
	else abort();

	if (!base_linkname.empty())
	{
		string const linkname = output_dir + "/" + base_linkname + attrs + ".png";

		unlink(linkname.c_str());
		if (symlink(filename.c_str(), linkname.c_str()))
			perror("symlink");
	}

	return filename;
}

void ImageMaker::add_job(string const & path, std::function<vector<Magick::Image>()> mk_frames)
{
	if (!boost::filesystem::exists(path))
		gif_generators.add_job(GifGenerationJob{path, mk_frames()});
}

string ImageMaker::rotation_gif(
	string const output_dir, Position p, ImageView const view,
	unsigned const width, unsigned const height, BgColor const bg_color,
	string const base_linkname)
{
	if (view.mirror) p = mirror(p);

	string const base_filename = to_string(hash_value(p)) + "rot" + to_string(bg_color);
	string const gif_filename = "store/" + base_filename + ".gif";

	string const linkname =
		base_linkname +
		to_string(width) + 'x' + to_string(height) +
		'c' + to_string(bg_color) + ".gif";

	double const ymax = std::max(.8, std::max(p[player0][Head].y, p[player1][Head].y));

	add_job(output_dir + "/" + gif_filename, [&]
		{
			vector<pair<Position, Camera>> pcs;

			for (auto i = 0; i < 360; i += 5)
			{
				double angle = i/180.*pi();

				Camera camera;
				camera.hardSetOffset({0, ymax - 0.57, 0});
				camera.zoom(0.55);
				camera.rotateHorizontal(angle);
				camera.rotateVertical((ymax - 0.6)/2);

				pcs.emplace_back(p, camera);
			}

			return make_frames(pcs, 8, width, height, color(bg_color), {0, 0, 1, 1, none, 45});
		});

	unlink((output_dir + "/" + linkname).c_str());
	if (symlink(gif_filename.c_str(), (output_dir + "/" + linkname).c_str()))
		perror("symlink");

	return linkname;
}

string ImageMaker::gif(
	string const output_dir,
	vector<Position> const & frames,
	ImageView const view,
	unsigned const width, unsigned const height,
	BgColor const bg_color, string const base_linkname)
{
	string const attrs = to_string(width) + 'x' + to_string(height) + code(view);

	string filename
		= "store/" + to_string(boost::hash_value(frames))
		+ attrs + '-' + to_string(bg_color) + ".gif";

	if (view.heading)
		add_job(output_dir + "/" + filename, [&]
			{
				vector<double> ymaxes;
				foreach (pos : frames)
					ymaxes.push_back(std::max(.8, std::max(pos[player0][Head].y, pos[player1][Head].y)));
				for (int i = 0; i != 10; ++i)
					ymaxes = smoothen_v(ymaxes);

				vector<pair<Position, Camera>> pcs;

				int i = 0;
				foreach (pos : frames)
				{
					auto qos = pos;
					if (view.mirror) qos = mirror(pos);

					double ymax = ymaxes[i];

					Camera camera;
					camera.hardSetOffset({0, ymax - 0.57, 0});
					camera.zoom(0.55);
					camera.rotateHorizontal(angle(*view.heading));
					camera.rotateVertical((ymax - 0.6)/2);

					pcs.emplace_back(qos, camera);

					++i;
				}

				return make_frames(pcs, 3, width, height, color(bg_color), {0, 0, 1, 1, none, 45});
			});

	if (base_linkname.empty())
		error("gifs must have name for symlink");

	string const linkname = output_dir + "/" + base_linkname + attrs + ".gif";

	unlink(linkname.c_str());
	if (symlink(filename.c_str(), linkname.c_str()))
		perror("symlink");

	return linkname;
}

string ImageMaker::gifs(
	string const output_dir,
	vector<Position> const & frames,
	unsigned const width, unsigned const height,
	BgColor const bg_color, string const base_linkname)
{
	string const
		attrs = to_string(width) + 'x' + to_string(height) + '-' + to_string(bg_color),
		base_filename = to_string(boost::hash_value(frames)) + attrs,
		ext_linkbase = base_linkname + attrs;

	foreach (view : views())
	{
		string const
			suffix = code(view) + string(".gif"),
			filename = "store/" + base_filename + suffix,
			linkname = output_dir + "/" + ext_linkbase + suffix;

		if (view.heading)
			add_job(output_dir + "/" + filename, [&]
				{
					vector<double> ymaxes;
					foreach (pos : frames)
						ymaxes.push_back(std::max(.8, std::max(pos[player0][Head].y, pos[player1][Head].y)));
					for (int i = 0; i != 10; ++i)
						ymaxes = smoothen_v(ymaxes);

					vector<pair<Position, Camera>> pcs;

					int i = 0;
					foreach (pos : frames)
					{
						double ymax = ymaxes[i];

						Camera camera;
						camera.hardSetOffset({0, ymax - 0.57, 0});
						camera.zoom(0.55);
						camera.rotateHorizontal(angle(*view.heading));
						camera.rotateVertical((ymax - 0.6)/2);

						pcs.emplace_back(pos, camera);

						++i;
					}

					return make_frames(pcs, 3, width, height, color(bg_color), {0, 0, 1, 1, none, 45});
				});

		unlink(linkname.c_str());
		if (symlink(filename.c_str(), linkname.c_str()))
			perror("symlink");
	}

	return ext_linkbase;
}

ImageMaker::ImageMaker(Graph const & g)
	: graph(g)
	, ctx(OSMesaCreateContextExt(OSMESA_RGB, 16, 0, 16, nullptr))
	, gif_generators(8)
{
	if (!ctx.context) error("OSMeseCreateContextExt failed");
}

void process(GifGenerationJob & job)
{
	Magick::writeImages(job.frames.begin(), job.frames.end(), job.path);
}

}
