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

constexpr size_t columns = 10;

void ImageMaker::make_mp4(
	string const filename,
	string const linkname,
	size_t const delay,
	unsigned const width, unsigned const height,
	V3 const bg_color,
	View const & view,
	function<vector<pair<Position, Camera>>()> make_pcs)
{
	store(filename, linkname, [&]
		{
			auto pcs = make_pcs();

			assert(!pcs.empty());
			if (no_anim) pcs.resize(1);

			size_t const
				n = pcs.size(),
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
			}

			glFlush();
			glFinish();

			foreach (p : buf) std::swap(p[0], p[2]);

			video_generators.add_job(VideoGenerationJob
				{ res_dir + "/store/" + filename
				, n, width, height
				, move(buf) });
		});
}


void process(VideoGenerationJob const & job, size_t const threadid)
{
	vector<boost::gil::rgb8_pixel_t> buf2(job.width * job.height);

	size_t const
		aawidth = job.width * 2,
		aaheight = job.height * 2;

	for (size_t i = 0; i != job.num_frames; ++i)
	{
		size_t const
			row = i / columns,
			column = i % columns;

		std::ostringstream oss;
		oss << "/tmp/t" << threadid << "frame" << std::setfill('0') << std::setw(3) << i << ".png";
		string const tmpfile = oss.str();

		auto xy = [&](unsigned x, unsigned y)
			{
				return job.tiled_frames[(row * aaheight + y) * (columns * aawidth) + (column * aawidth + x)];
			};

		for (unsigned x = 0; x != job.width; ++x)
		for (unsigned y = 0; y != job.height; ++y)
		{
			auto const & a = xy(x*2,  y*2  );
			auto const & b = xy(x*2+1,y*2  );
			auto const & c = xy(x*2,  y*2+1);
			auto const & d = xy(x*2+1,y*2+1);

			buf2[y*job.width+x][0] = (a[0] + b[0] + c[0] + d[0]) / 4;
			buf2[y*job.width+x][1] = (a[1] + b[1] + c[1] + d[1]) / 4;
			buf2[y*job.width+x][2] = (a[2] + b[2] + c[2] + d[2]) / 4;
		}

		boost::gil::png_write_view(tmpfile,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(job.width, job.height, buf2.data(), job.width*3)));
	}

	string command = "ffmpeg -threads 1 -y -loglevel panic -i '/tmp/t" + to_string(threadid) + "frame%03d.png' -frames:v "
		+ std::to_string(job.num_frames) + "  -c:v libx264 -pix_fmt yuv420p -movflags +faststart "
		+ job.output_file.native();

	if (std::system(command.c_str()) != 0)
		throw std::runtime_error("command failed: " + command);

	cout << '.' << std::flush;
}

void ImageMaker::png(
	pair<Position, Camera> const * pos_b,
	pair<Position, Camera> const * pos_e,
	unsigned const width, unsigned const height,
	string const path, V3 const bg_color,
	vector<View> const & view,
	unsigned const grid_size, unsigned const grid_line_width)
{
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
	string const path,
	unsigned const width, unsigned const height, V3 const bg_color)
{
	Camera camera;
	camera.hardSetOffset({0, ymax - 0.57, 0});
	camera.zoom(0.55);
	camera.rotateHorizontal(angle);
	camera.rotateVertical((ymax - 0.6)/2);

	png(pos, camera, width, height, path, bg_color, {{0, 0, 1, 1, none, 45}});
}

void ImageMaker::store(
	string const & filename,
	string const & linkname,
	std::function<void()> write_file)
{
	bool const
		file_exists = elem(filename, stored_initially),
		link_exists = elem(linkname, linked_initially);

	if (file_exists && link_exists) return;

	string const link_target = "store/" + filename;
	string const link_path = res_dir + "/" + linkname;

	unlink(link_path.c_str());

	write_file();

	if (symlink(link_target.c_str(), link_path.c_str()))
		perror("symlink");
}

void ImageMaker::png(
	Position pos,
	double const ymax,
	ImageView const view,
	unsigned const width, unsigned const height,
	BgColor const bg_color, string const base_linkname)
{
	string const
		attrs = code(view) + to_string(width) + 'x' + to_string(height),
		filename = to_string(hash_value(pos)) + attrs + '-' + to_string(bg_color) + ".png",
		linkname = base_linkname + attrs + ".png";

	store(filename, linkname, [&]
	{
		if (view.mirror) pos = mirror(pos);

		string const path = res_dir + "/store/" + filename;

		if (view.heading)
		{
			png(pos, angle(*view.heading), ymax, path, width, height, color(bg_color));
		}
		else if (view.player)
		{
			Camera camera;

			png(pos, camera, width, height, path, color(bg_color),
				{{0, 0, 1, 1, *view.player, 80}});
		}
		else abort();
	});
}

string ImageMaker::rotation_gif(
	Position p, ImageView const view,
	unsigned const width, unsigned const height, BgColor const bg_color,
	string const base_linkname)
{
	if (view.mirror) p = mirror(p);

	string const base_filename = to_string(hash_value(p)) + "rot" + to_string(bg_color);
	string const mp4_filename = base_filename + ".mp4";

	string const linkname =
		base_linkname +
		to_string(width) + 'x' + to_string(height) +
		'c' + to_string(bg_color) + ".mp4";

	make_mp4(mp4_filename, linkname, 8, width, height, color(bg_color), {0, 0, 1, 1, none, 45},
		[&]
		{
			double const ymax = std::max(.8, std::max(p[player0][Head].y, p[player1][Head].y));

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

			return pcs;
		});

	return linkname;
}

string ImageMaker::gif(
	vector<Position> const & frames,
	ImageView const view,
	unsigned const width, unsigned const height,
	BgColor const bg_color, string const base_linkname)
{
	if (base_linkname.empty())
		error("gifs must have name for symlink");

	string const attrs = to_string(width) + 'x' + to_string(height) + code(view);

	string filename
		= to_string(boost::hash_value(frames))
		+ attrs + '-' + to_string(bg_color) + ".mp4";

	string const linkname = base_linkname + attrs + ".mp4";

	if (view.heading)
		make_mp4(filename, linkname, 3, width, height, color(bg_color), {0, 0, 1, 1, none, 45},
			[&]
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

				return pcs;
			});

	return linkname;
}

string ImageMaker::gifs(
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
			suffix = code(view) + string(".mp4"),
			filename = base_filename + suffix,
			linkname = ext_linkbase + suffix;

		if (view.heading)
			make_mp4(filename, linkname, 3, width, height, color(bg_color), {0, 0, 1, 1, none, 45},
				[&]
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

						if (view.mirror)
							pcs.back().first = mirror(pcs.back().first);

						++i;
					}

					return pcs;
				});
	}

	return ext_linkbase;
}

ImageMaker::ImageMaker(Graph const & g, string rd)
	: graph(g)
	, ctx(OSMesaCreateContextExt(OSMESA_RGB, 16, 0, 16, nullptr))
	, video_generators(1)
    , res_dir(rd)
{
	for (fs::directory_iterator i(res_dir + "/store"), e; i != e; ++i)
		stored_initially.insert(fs::path(*i).filename().native());

	for (fs::directory_iterator i(res_dir), e; i != e; ++i)
		if (is_symlink(*i))
			linked_initially.insert(fs::path(*i).filename().native());

	cout << "Found " << stored_initially.size() << " existing items and "
		<< linked_initially.size() << " existing links in store.\n";

	if (!ctx.context) error("OSMeseCreateContextExt failed");
}

void ImageMaker::make_svg(string const & filename, string const & dot) const
{
	string
		dotpath = res_dir + "/tmp.dot",
		svgpath = res_dir + "/store/" + filename;
	
	{ std::ofstream dotfile(dotpath); dotfile << dot; }

	FILE * fp = fopen(dotpath.c_str(), "r");
	FILE * op = fopen(svgpath.c_str(), "w");
	Agraph_t *g = agread(fp, 0);
	gvLayout(gvc, g, "dot");
	gvRender(gvc, g, "svg", op);
	gvFreeLayout(gvc, g);
	agclose(g);
	fclose(fp);
	fclose(op);

	// doing this in-process rather than with std::system ensures that
	// mkpospages sees ctrl-C and can handle it gracefully
}

}
