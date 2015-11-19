
#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph.hpp"
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>

#define int_p_NULL (int*)NULL // https://github.com/ignf/gilviewer/issues/8

#include <boost/gil/extension/io/png_io.hpp>
#include <boost/gil/gil_all.hpp>

namespace
{
	struct Config
	{
		string db;
	};

	optional<Config> config_from_args(int const argc, char const * const * const argv)
	{
		namespace po = boost::program_options;

		po::options_description desc("options");
		desc.add_options()
			("help", "show this help")
			("db", po::value<string>()->default_value("positions.txt"),
				"position database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { std::cout << desc << '\n'; return none; }

		return Config{ vm["db"].as<string>() };
	}

	unsigned const width = 320, height = 240;

	void make_png(GLFWwindow * const window, Graph const & graph, std::string const & name, Position const & pos)
	{
		Camera camera;
		camera.zoom(-0.5);

		Style style;
		style.background_color = white;

		renderWindow(
			{ {0, 0, 1, 1, none, 90} }, // views
			nullptr, // no viables
			graph, window, pos, camera,
			none, // no highlighted joint
			false, // not edit mode
			width, height, style);

		boost::gil::rgb8_pixel_t buf[width * height];

		glFlush();
		glFinish();

		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

		boost::gil::png_write_view(name,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf, width*3)));
	}

	void make_gif(GLFWwindow * const window, Graph const & graph, std::string const & name, std::vector<Position> const & frames)
	{
		std::cout << name << ": " << frames.size() << std::endl;

		int i = 0;

		foreach (pos : frames)
		{
			std::ostringstream f;
			f << "browse/" << name << '-' << std::setw(3) << std::setfill('0') << i << ".png";

			make_png(window, graph, f.str(), pos);

			++i;
		}

		std::system(("convert -depth 8 -delay 4 -loop 0 'browse/" + name + "-*.png' browse/" + name).c_str());
	}

	std::vector<Position> frames_for_sequence(Graph const & graph, SeqNum const seqNum)
	{
		unsigned const frames_per_pos = 5;

		PositionInSequence location{seqNum, 0};

		std::vector<Position> r(10, graph[location]);

		for (; next(graph, location); location = *next(graph, location))
			for (unsigned howfar = 0; howfar != frames_per_pos; ++howfar)
				r.push_back(between(
					graph[location],
					graph[*next(graph, location)],
					howfar / double(frames_per_pos)));

		r.resize(r.size() + 10, graph[seqNum].positions.back());

		return r;
	}
}

std::string img(Graph const & g, SeqNum const sn, bool const incoming)
{
	return std::string("<img src='") + (incoming ? "in" : "out") + std::to_string(sn.index) + ".gif' title='" + g[sn].description + "'></img>";
}

std::string img(NodeNum const n)
{
	std::ostringstream oss;
	oss << "<img src='" << n << ".png'></img>";
	return oss.str();
}

void write_index(Graph const & graph)
{
	std::ofstream html("browse/index.html");

	html
		<< "<html><body>"
		<< "<table>";

	foreach (p : nodes(graph))
	{
		NodeNum const n = p.first;

		html
			<< "<tr><td colspan='3'><hr></td></tr>\n"
			<< "<tr><td><table style='float:right'>\n";

		foreach (sn : p.second.first)
			html
				<< "<tr>"
				<< "<td style='text-align:right'>" << graph[sn].description << "</td>"
				<< "<td><a href='#" << graph.from(sn).node << "'>" << img(graph, sn, true) << "</a></td>"
				<< "</tr>";

		html
			<< "</table></td>"
			<< "<td id='" << n << "'>" << img(n) << "</td>"
			<< "<td><table>";

		foreach (sn : p.second.second)
			html
				<< "<tr>"
				<< "<td><a href='#" << graph.to(sn).node << "'>" << img(graph, sn, false) << "</a></td>"
				<< "<td>" << graph[sn].description << "</td>"
				<< "</tr>\n";

		html << "</table></td></tr>\n\n";
	}

	html << "</table></body></html>";
}

void write_node_pages(Graph const & graph)
{

	foreach (p : nodes(graph))
	{
		std::ofstream html("browse/node" + std::to_string(p.first.index) + ".html");

		html
			<< "<html><body>"
			<< "<table>";

		NodeNum const n = p.first;

		html
			<< "<tr><td><table style='float:right'>\n";

		foreach (sn : p.second.first)
			html
				<< "<tr>"
				<< "<td style='text-align:right'>" << graph[sn].description << "</td>"
				<< "<td><a href='" << graph.from(sn).node << ".html'>" << img(graph, sn, true) << "</a></td>"
				<< "</tr>";

		html
			<< "</table></td>"
			<< "<td id='" << n << "'>" << img(n) << "</td>"
			<< "<td><table>";

		foreach (sn : p.second.second)
			html
				<< "<tr>"
				<< "<td><a href='" << graph.to(sn).node << ".html'>" << img(graph, sn, false) << "</a></td>"
				<< "<td>" << graph[sn].description << "</td>"
				<< "</tr>\n";

		html << "</table></td></tr>\n\n";
		html << "</table></body></html>";
	}

}

int main(int const argc, char const * const * const argv)
{
	try
	{
		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		Graph const graph(load(config->db));

		if (!glfwInit()) error("could not initialize GLFW");

		// glfwWindowHint(GLFW_VISIBLE, GL_FALSE); // TODO: figure out how to make this work

		GLFWwindow * const window = glfwCreateWindow(width, height, "Jiu Jitsu Map", nullptr, nullptr);

		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);

		write_index(graph);
		write_node_pages(graph);

		foreach (p : nodes(graph))
		{
			NodeNum const n = p.first;

			std::cout << n << std::endl;

			auto pos = graph[n];
			V2 const center = xz(pos[0][Core] + pos[1][Core]) / 2;

			PositionReorientation reo;
			reo.reorientation.offset.x = -center.x;
			reo.reorientation.offset.z = -center.y;

			make_png(window, graph, "browse/node" + std::to_string(n.index) + ".png", reo(pos));

			foreach (in_sn : p.second.first)
			{
				auto v = frames_for_sequence(graph, in_sn);
				foreach (p : v) p = reo(inverse(graph.to(in_sn).reorientation)(p));

				assert(basicallySame(v.back(), reo(pos)));

				make_gif(window, graph, "in" + std::to_string(in_sn.index) + ".gif", v);
			}

			foreach (out_sn : p.second.second)
			{
				auto v = frames_for_sequence(graph, out_sn);
				foreach (p : v) p = reo(inverse(graph.from(out_sn).reorientation)(p));

				assert(basicallySame(v.front(), reo(pos)));

				make_gif(window, graph, "out" + std::to_string(out_sn.index) + ".gif", v);
			}
		}
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
