
#include "images.hpp"
#include "camera.hpp"
#include "rendering.hpp"
#include <boost/program_options.hpp>

inline std::size_t hash_value(V3 const v) // todo: put elsewhere
{
	size_t seed = 0;
	boost::hash_combine(seed, v.x);
	boost::hash_combine(seed, v.y);
	boost::hash_combine(seed, v.z);
	return seed;
}

#include <boost/functional/hash.hpp>
#define int_p_NULL (int*)NULL // https://github.com/ignf/gilviewer/issues/8

#include <boost/gil/extension/io/png_io.hpp>
#include <boost/gil/gil_all.hpp>
#include <boost/filesystem.hpp>

namespace
{
	void make_gif(
		string const output_dir,
		string const filename,
		unsigned const delay,
		std::function<vector<string>(string)> make_frames)
	{
		if (!boost::filesystem::exists(output_dir + filename))
		{
			string const gif_frames_dir = output_dir + "gifframes/";

			string command = "convert -depth 8 -delay " + to_string(delay) + " -loop 0 ";
			foreach (f : make_frames(gif_frames_dir)) command += gif_frames_dir + f + ' ';
			command += output_dir + filename;

			if (std::system(command.c_str()) != 0)
				throw std::runtime_error("command failed: " + command);
		}
	}

	void make_png(
		Graph const & graph,
		#if USE_OSMESA
			OSMesaContext const mesa,
		#else
			GLFWwindow * window,
		#endif
		Position pos,
		Camera const & camera,
		unsigned width, unsigned height,
		string const path, V3 const bg_color, View const view)
	{
		if (boost::filesystem::exists(path)) return;

		vector<boost::gil::rgb8_pixel_t> buf(width * height * 3);

		#if USE_OSMESA
			if (!OSMesaMakeCurrent(mesa, buf.data(), GL_UNSIGNED_BYTE, width, height))
				error("OSMesaMakeCurrent");
		#endif

		Style style;
		style.grid_color = bg_color * .8;
		style.background_color = bg_color;

		renderWindow(
			{view},
			nullptr, // no viables
			graph, pos, camera,
			none, // no highlighted joint
			false, // not edit mode
			0, 0, width, height,
			{0},
			style);

		#if !USE_OSMESA
			glfwSwapBuffers(window);
		#endif

		glFlush();
		glFinish();

		#if !USE_OSMESA
			glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf.data());
		#endif

		boost::gil::png_write_view(path,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf.data(), width*3)));

		std::cout << "wrote " << path << '\n';
	}
}

void ImageMaker::png(
	string const output_dir,
	Position const pos,
	double const angle,
	string const filename,
	unsigned const width, unsigned const height, V3 const bg_color) const
{
	if (!boost::filesystem::exists(output_dir + filename))
	{
		double const ymax = std::max(.8, std::max(pos[0][Head].y, pos[1][Head].y));

		Camera camera;
		camera.hardSetOffset({0, ymax - 0.57, 0});
		camera.zoom(0.55);
		camera.rotateHorizontal(angle);
		camera.rotateVertical((ymax - 0.6)/2);

		make_png(graph,
			#if USE_OSMESA
				ctx,
			#else
				window,
			#endif
			pos, camera, width, height, output_dir + filename, bg_color, {0, 0, 1, 1, none, 45});
	}
}

string ImageMaker::png(
	string const output_dir,
	Position pos,
	ImageView const view,
	unsigned const width, unsigned const height,
	BgColor const bg_color) const
{
	string const filename =
		to_string(boost::hash_value(pos))
		+ code(view)
		+ to_string(width) + 'x' + to_string(height)
		+ '-' + to_string(bg_color)
		+ ".png";

	if (view.mirror) pos = mirror(pos);

	if (view.heading)
		png(output_dir, pos, angle(*view.heading), filename, width, height, color(bg_color));
	else if (view.player)
	{
		Camera camera;

		make_png(graph,
			#if USE_OSMESA
				ctx,
			#else
				window,
			#endif
			pos, camera, width, height, output_dir + filename, color(bg_color),
			{0, 0, 1, 1, *view.player, 80});
	}
	else abort();

	return filename;
}

string ImageMaker::rotation_gif(
	string const output_dir, Position p, ImageView const view,
	unsigned const width, unsigned const height, BgColor const bg_color) const
{
	if (view.mirror) p = mirror(p);

	string const base_filename = to_string(boost::hash_value(p)) + "rot" + to_string(bg_color);
	string const gif_filename = base_filename + ".gif";

	make_gif(output_dir, gif_filename, 8, [&](string const gif_frames_dir)
		{
			vector<string> frames;

			for (auto i = 0; i < 360; i += 5)
			{
				string const frame_filename = base_filename + "-" + to_string(i) + "-" + to_string(bg_color) + ".png";
				png(gif_frames_dir, p, i/180.*M_PI, frame_filename, width, height, color(bg_color));
				frames.push_back(frame_filename);
			}

			return frames;
		});

	return gif_filename;
}

string ImageMaker::gif(
	string const output_dir,
	vector<Position> const & frames,
	ImageView const view,
	unsigned const width, unsigned const height, BgColor const bg_color) const
{
	string const filename
		= to_string(boost::hash_value(frames))
		+ to_string(width) + 'x' + to_string(height)
		+ code(view)
		+ '-' + to_string(bg_color)
		+ ".gif";

	make_gif(output_dir, filename, 3, [&](string const gif_frames_dir)
		{
			vector<string> v;
			foreach (pos : frames) v.push_back(png(gif_frames_dir, pos, view, width, height, bg_color));
			return v;
		});

	return filename;
}

string ImageMaker::gifs(
	string const output_dir,
	vector<Position> const & frames,
	unsigned const width, unsigned const height, BgColor bg_color) const
{
	string const base_filename
		= to_string(boost::hash_value(frames))
		+ to_string(width) + 'x' + to_string(height)
		+ '-' + to_string(bg_color);

	foreach (view : views())
		make_gif(output_dir, base_filename + code(view) + ".gif", 3, [&](string const gif_frames_dir)
			{
				vector<string> v;
				foreach (pos : frames) v.push_back(png(gif_frames_dir, pos, view, width, height, bg_color));
				return v;
			});

	return base_filename;
}

#if USE_OSMESA
	ImageMaker::ImageMaker(Graph const & g)
		: graph(g)
		, ctx(OSMesaCreateContextExt(OSMESA_RGB, 16, 0, 0, nullptr))
	{
		if (!ctx) error("OSMeseCreateContextExt failed");
	}
#else
	ImageMaker::ImageMaker(Graph const & g)
		: graph(g)
	{
		if (!glfwInit()) error("could not initialize GLFW");

//		glfwWindowHint(GLFW_VISIBLE, GL_FALSE); // TODO: figure out how to make this work

		window = glfwCreateWindow(640, 480, "Jiu Jitsu Map", nullptr, nullptr);

		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);
	}
#endif

ImageMaker::~ImageMaker()
{
	#if USE_OSMESA
		OSMesaDestroyContext(ctx);
	#endif
}

