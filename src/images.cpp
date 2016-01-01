
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
	char const headings[4] = {'n', 'e', 's', 'w'};
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
		camera.hardSetOffset({0, ymax - 0.6, 0});
		camera.zoom(0.6);
		camera.rotateHorizontal(angle);
		camera.rotateVertical((ymax - 0.6)/2);

		Style style;
		style.background_color = bg_color;

		renderWindow(
			{ {0, 0, 1, 1, none, 45} }, // views
			nullptr, // no viables
			graph, window, pos, camera,
			none, // no highlighted joint
			false, // not edit mode
			width, height,
			{0},
			style);

		glfwSwapBuffers(window);

		boost::gil::rgb8_pixel_t buf[width * height];

		glFlush();
		glFinish();

		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

		boost::gil::png_write_view(output_dir + filename,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf, width*3)));
	}
}

string ImageMaker::png(
	string const output_dir,
	Position const pos,
	unsigned const heading,
	unsigned const width, unsigned const height, BgColor const bg_color) const
{
	string const filename
		= to_string(boost::hash_value(pos))
		+ headings[heading]
		+ to_string(width) + 'x' + to_string(height)
		+ '-' + to_string(bg_color)
		+ ".png";

	png(output_dir, pos, M_PI * 0.5 * heading, filename, width, height, color(bg_color));

	return filename;
}

string ImageMaker::rotation_gif(
	string const output_dir, Position const p,
	unsigned const width, unsigned const height, BgColor const bg_color) const
{
	string const base_filename = to_string(boost::hash_value(p)) + "rot" + to_string(bg_color);

	string const gif_filename = base_filename + ".gif";

	if (!boost::filesystem::exists(output_dir + gif_filename))
	{
		vector<string> png_files;
		string const gif_frames_dir = output_dir + "gifframes/";

		for (auto i = 0; i < 360; i += 5)
		{
			string const frame_filename = base_filename + "-" + to_string(i) + "-" + to_string(bg_color) + ".png";
			png(gif_frames_dir, p, i/180.*M_PI, frame_filename, width, height, color(bg_color));
			png_files.push_back(frame_filename);
		}

		string command = "convert -depth 8 -delay 9 -loop 0 ";
		foreach (f : png_files) command += gif_frames_dir + f + ' ';
		command += output_dir + gif_filename;

		std::system(command.c_str());
	}

	return gif_filename;
}

string ImageMaker::gif(
	string const output_dir,
	vector<Position> const & frames,
	unsigned const heading,
	unsigned const width, unsigned const height, BgColor const bg_color) const
{
	string const filename
		= to_string(boost::hash_value(frames))
		+ to_string(width) + 'x' + to_string(height)
		+ headings[heading]
		+ '-' + to_string(bg_color)
		+ ".gif";

	if (!boost::filesystem::exists(output_dir + filename))
	{
		vector<string> png_files;
		string const gif_frames_dir = output_dir + "gifframes/";
		foreach (pos : frames) png_files.push_back(png(gif_frames_dir, pos, heading, width, height, bg_color));

		string command = "convert -depth 8 -delay 3 -loop 0 ";
		foreach (f : png_files) command += gif_frames_dir + f + ' ';
		command += output_dir + filename;

		std::system(command.c_str());
	}

	return filename;
}

string ImageMaker::gifs(
	string const output_dir,
	vector<Position> const & frames,
	unsigned const width, unsigned const height, BgColor bg_color) const
{
	string const base_filename
		= to_string(boost::hash_value(frames))
		+ to_string(width) + 'x' + to_string(height);

	for (unsigned heading = 0; heading != 4; ++heading)
	{
		string const filename = base_filename + headings[heading] + ".gif";

		if (!boost::filesystem::exists(output_dir + filename))
		{
			vector<string> png_files;
			string const gif_frames_dir = output_dir + "gifframes/";
			foreach (pos : frames) png_files.push_back(png(gif_frames_dir, pos, heading, width, height, bg_color));

			string command = "convert -depth 8 -delay 3 -loop 0 ";
			foreach (f : png_files) command += gif_frames_dir + f + ' ';
			command += output_dir + filename;

			std::system(command.c_str());
		}
	}

	return base_filename;
}
