
#include "util.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph_util.hpp"
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <boost/program_options.hpp>
#include <boost/range/distance.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>

#define int_p_NULL (int*)NULL // https://github.com/ignf/gilviewer/issues/8

#include <boost/gil/extension/io/png_io.hpp>
#include <boost/gil/gil_all.hpp>

namespace
{
	char const headings[4] = {'n', 'e', 's', 'w'};

	struct Config
	{
		string db;
		bool nogifs;
		optional<NodeNum> node;
	};

	optional<Config> config_from_args(int const argc, char const * const * const argv)
	{
		namespace po = boost::program_options;

		po::options_description desc("options");
		desc.add_options()
			("help",
				"show this help")
			("nogifs",
				po::value<bool>()->default_value(false),
				"instead of animated gifs, only show midpoints (the gifs take ages)")
			("position",
				po::value<uint16_t>(),
				"only generate position page for specified position")
			("db",
				po::value<string>()->default_value("GrappleMap.txt"),
				"database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { std::cout << desc << '\n'; return none; }

		Config cfg{
			vm["db"].as<string>(),
			vm["nogifs"].as<bool>(),
			boost::none};

		if (vm.count("position"))
			cfg.node = NodeNum{vm["position"].as<uint16_t>()};

		return cfg;
	}

	void make_png(GLFWwindow * const window, Graph const & graph, std::string const & name, Position const & pos, unsigned const width, unsigned const height, unsigned const heading)
	{
		Camera camera;
		camera.zoom(0.6);

		camera.rotateHorizontal(M_PI * 0.5 * heading);

		Style style;
		style.background_color = white;

		renderWindow(
			{ {0, 0, 1, 1, none, 45} }, // views
			nullptr, // no viables
			graph, window, pos, camera,
			none, // no highlighted joint
			false, // not edit mode
			width, height,
			{0},
			style);

		boost::gil::rgb8_pixel_t buf[width * height];

		glFlush();
		glFinish();

		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

		boost::gil::png_write_view(name,
			boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf, width*3)));
	}

	void make_gif(
		GLFWwindow * const window, Graph const & graph, string const & name,
		vector<Position> const & frames, unsigned const width, unsigned const height, unsigned const heading)
	{
		std::cout << ' ' << name << ": " << frames.size() << std::endl;

		int i = 0;

		foreach (pos : frames)
		{
			std::ostringstream f;
			f << "browse/" << name << headings[heading] << '-' << std::setw(3) << std::setfill('0') << i << ".png";

			make_png(window, graph, f.str(), pos, width, height, heading);

			++i;
		}

		std::system(("convert -depth 8 -delay 3 -loop 0 'browse/" + name + headings[heading] + "-*.png' browse/" + name + ".gif").c_str());
	}

	vector<Position> frames_for_sequence(Graph const & graph, SeqNum const seqNum)
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

	string desc(Graph const & g, NodeNum const n)
	{
		auto desc = g[n].description;

		if (desc.empty()) return to_string(n.index);

		return desc.front();
	}

	string img(NodeNum const n, char const hc)
	{
		auto s = to_string(n.index);
		return "<img alt='' title='" + s + "' src='node" + s + hc + ".png'>";
	}

	string desc(Graph const & g, SeqNum const s)
	{
		auto desc = replace_all(g[s].description.front(), "\\n", "<br>");
		if (desc == "...") desc.clear();
		if (!desc.empty()) desc += "<br>";
		return desc;
	}

	string const html5head =
		"<!DOCTYPE html>"
		"<html lang='en'>"
		"<head><title>GrappleMap</title>"
		"<meta charset='UTF-8'/>"
		"</head>";

	void write_todo(Graph const & g)
	{
		std::ofstream html("browse/todo.html");

		html << "</ul><h2>Dead ends</h2><ul>";

		auto nlspace = [](string const & s){ return replace_all(s, "\\n", " "); };

		foreach(n : nodenums(g))
			if (out(g, n).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul><h2>Dead starts</h2><ul>";

		foreach(n : nodenums(g))
			if (in(g, n).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul><h2>Untagged positions</h2><ul>";

		foreach(n : nodenums(g))
			if (tags(g, n).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul></body></html>";
	}

	void write_index(Graph const & g)
	{
		std::ofstream html("browse/index.html");

		html << html5head << "<body><h1>Index</h1><h2>Tags (" << tags(g).size() << ")</h2><ul>";

		foreach(tag : tags(g))
		{
			set<NodeNum> s;
			foreach(n : tagged_nodes(g, tag)) s.insert(n);
				// i can't figure out how to get the size of the tagged_nodes result directly

			html << "<li><a href='tag-" << tag << ".html'>" << tag << "</a> (" << s.size() << ")</li>";
		}

		html << "</ul><h2>Positions (" << g.num_nodes() << ")</h2><ul>";

		auto nlspace = [](string const & s){ return replace_all(s, "\\n", " "); };

		foreach(n : nodenums(g))
			html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul><h2>Transitions (" << g.num_sequences() << ")</h2><ul>";

		foreach(s : seqnums(g))
		{
			html
				<< "<li><em>from</em>"
				<< " <a href='p" << g.from(s).node.index << "n.html'>" << nlspace(desc(g, g.from(s).node)) << "</a>";

			if (g[s].description.front() != "...")
				html
					<< " <em>via</em> "
					<< "<b>" << nlspace(g[s].description.front()) << "</b>";

			html
				<< " <em>to</em>"
				<< " <a href='p" << g.to(s).node.index << "n.html'>" << nlspace(desc(g, g.to(s).node)) << "</a>"
				<< "</li>";
		}

		html << "</ul></body></html>";
	}

	string make_svg(Graph const & g, map<NodeNum, bool> const & nodes)
	{
		string const
			svgpath = "browse/tmp.svg",
			dotpath = "browse/tmp.dot";

		{
			std::ofstream dotfile(dotpath);
			todot(g, dotfile, nodes);
		}

		auto cmd = "dot -Tsvg " + dotpath + " -o" + svgpath;
		std::system(cmd.c_str());

		std::ifstream svgfile(svgpath);
		std::istreambuf_iterator<char> i(svgfile), e;
		string const r(i, e);

		return r.substr(r.find("<svg"));
	}

	void write_tag_page(Graph const & g, string const & tag, bool const nogifs)
	{
		std::ofstream html("browse/tag-" + tag + ".html");

		html << html5head << "<body style='text-align:center'><h1>Tag: " << tag << "</h1><hr>";

		// positions

		html << "<h2>Positions</h2><p>Positions tagged '" << tag << "'</p>";

		set<NodeNum> nodes;

		foreach(n : tagged_nodes(g, tag))
		{
			html
				<< "<div style='display:inline-block;text-align:center'>"
				<< "<a href='p" << n.index << "e.html'>"
				<< replace_all(desc(g, n), "\\n", "<br>") << "<br>"
				<< "<img alt='' title='" << n.index << "' src='p" << n.index << "w.png'></a>"
				<< "</div>";

			nodes.insert(n);
		}

		map<NodeNum, bool> m;
		foreach(n : nodes) m[n] = true;
		foreach(n : nodes_around(g, nodes)) m[n] = false;

		html << "<br><a href='index.html'>Index</a>";

		// transitions

		html << "<hr><h2>Transitions</h2><p>Transitions that preserve the '" << tag << "' tag</p><table style='margin:0px auto'>\n";

		foreach(sn : seqnums(g))
			if (is_internal(g, nodes, sn))
			{
				auto const
					from = g.from(sn).node,
					to = g.to(sn).node;

				html
					<< "<tr><td style='text-align:right'><em>from</em> "
					<< "<a href='p" << from.index << "w.html'>"
					<< replace_all(desc(g, from), "\\n", " ")
					<< "</a> <em>via</em></td>"
					<< "<td><div style='display:inline-block'>"
					<< desc(g, sn)
					<< "<img alt='' src='in" << sn.index << "w." << (nogifs ? "png" : "gif") << "' title='" << sn.index;

				if (auto ln = g[sn].line_nr) html << " @ " << *ln;

				html
					<< "'></div></td><td style='text-align:left'><em>to</em> "
					<< "<a href='p" << to.index << "w.html'>"
					<< replace_all(desc(g, to), "\\n", " ")
					<< "</a>";
				
				html << "</td></tr>";
			}

		html << "</table>";


		html
			<< "<hr><h2>Neighbourhood</h2>"
			<< "<p>Positions up to one transition away from position";

		if (nodes.size() > 1) html << 's';

		html << " tagged '" << tag << "' (clickable)</p>" << make_svg(g, m) << "</body></html>";
	}

	void write_position_page(
		GLFWwindow * const window,
		Graph const & graph,
		NodeNum const n,
		bool const nogifs)
	{
		string const pname = "p" + to_string(n.index);

		std::cout << pname << std::endl;

		set<NodeNum> nodes{n};

		map<NodeNum, bool> m;
		m[n] = true;
		foreach(nn : nodes_around(graph, nodes, 2)) m[nn] = false;

		string const svg = make_svg(graph, m);

		vector<SeqNum> const
			incoming = in(graph, n),
			outgoing = out(graph, n);

		for (unsigned heading = 0; heading != 4; ++heading)
		{
			char const hc = headings[heading];

			auto pos = graph[n].position;
			V2 const center = xz(pos[0][Core] + pos[1][Core]) / 2;

			PositionReorientation reo;
			reo.reorientation.offset.x = -center.x;
			reo.reorientation.offset.z = -center.y;

			make_png(window, graph, "browse/p" + to_string(n.index) + hc + ".png", reo(pos), 480, 360, heading);

			std::ofstream html("browse/" + pname + hc + ".html");

			html << html5head << "<body style='text-align:center'><table style='margin:0px auto'><tr>";

			auto transition_img = [&](SeqNum const sn, string const & inout)
				{
					html
						<< "<img alt='' src='" << inout << sn.index << hc << "." << (nogifs ? "png" : "gif") << "' title='transition " << sn.index;
						
					if (auto ln = graph[sn].line_nr) html << " @ line " << *ln;
					auto d = graph[sn].description;
					d.erase(d.begin());
					foreach (l : d) html << '\n' << replace_all(l, "'", "\\'");

					html << "'>";
				};

			if (!incoming.empty())
			{
				html << "<td style='text-align:center;vertical-align:top'><b>Incoming transitions</b><table>\n";

				size_t n = 0;
				foreach (sn : incoming)
					n = std::max(n, frames_for_sequence(graph, sn).size());

				foreach (sn : incoming)
				{
					auto const from = graph.from(sn).node;

					html
						<< "<tr><td><hr>"
						<< "<em>from</em> <div style='display:inline-block'>"
						<< "<a href='p" << from.index << hc << ".html'>"
						<< replace_all(desc(graph, from), "\\n", "<br>") << "<br>"
						<< "<img alt='' title='" << from.index << "' src='from" << sn.index << hc << ".png'>"
						<< "</a></div> <em>via</em> <div style='display:inline-block'>"
						<< desc(graph, sn);

					transition_img(sn, "in");

					html << "</div> <em>to</em></td></tr>";

					auto v = frames_for_sequence(graph, sn);
					foreach (p : v) p = reo(inverse(graph.to(sn).reorientation)(p));
					assert(basicallySame(v.back(), reo(pos)));

					auto const p = v.front();
					make_png(window, graph, "browse/from" + to_string(sn.index) + hc + ".png", p, 160, 120, heading);

					if (!nogifs)
					{
						v.insert(v.begin(), n - v.size(), p);
						make_gif(window, graph, "in" + to_string(sn.index) + hc, v, 160, 120, heading);
					}
					else
						make_png(window, graph, "browse/in" + to_string(sn.index) + hc + ".png", v[v.size() / 2], 160, 120, heading);
				}

				html << "</table></td>";
			}

			char const next_heading = headings[(heading + 1) % 4];
			char const prev_heading = headings[(heading + 3) % 4];

			html
				<< "<td style='text-align:center;vertical-align:top'><h3>Position:</h3><h1>"
				<< replace_all(desc(graph, n), "\\n", "<br>") << "<br><br>"
				<< "<img alt='' title='" << n.index << "' src='p" << n.index << hc << ".png'><br>"
				<< "<a href='" << pname << prev_heading << ".html'>↻</a> "
				<< "<a href='" << pname << next_heading << ".html'>↺</a> "
				<< "</h1>";
			
			auto const t = tags(graph, n);
			if (!t.empty())
			{
				html << "Tags:";

				foreach(tag : tags(graph, n))
					html << " <a href='tag-" << tag << ".html'>" << tag << "</a>";

				html << "<br><br>";
			}

			html << "<a href='index.html'>Index</a></td>";

			if (!outgoing.empty())
			{
				html << "<td style='text-align:center;vertical-align:top'><b>Outgoing transitions</b><table>";

				size_t n = 0;
				foreach (sn : outgoing)
					n = std::max(n, frames_for_sequence(graph, sn).size());

				foreach (sn : outgoing)
				{
					auto const to = graph.to(sn).node;

					html
						<< "<tr><td><hr>"
						<< "<em>via</em> <div style='display:inline-block'>"
						<< desc(graph, sn);

					transition_img(sn, "out");

					html
						<< "</div> <em>to</em> <div style='display:inline-block'>"
						<< "<a href='p" << to.index << hc << ".html'>"
						<< replace_all(desc(graph, to), "\\n", "<br>") << "<br>"
						<< "<img alt='' title='" << to.index << "' src='to" << sn.index << hc << ".png'>"
						<< "</a></div></td></tr>";

					auto v = frames_for_sequence(graph, sn);
					foreach (p : v) p = reo(inverse(graph.from(sn).reorientation)(p));
					assert(basicallySame(v.front(), reo(pos)));

					auto const p = v.back();
					make_png(window, graph, "browse/to" + to_string(sn.index) + hc + ".png", p, 160, 120, heading);

					if (!nogifs)
					{
						v.insert(v.end(), n - v.size(), p);
						make_gif(window, graph, "out" + to_string(sn.index) + hc, v, 160, 120, heading);
					}
					else
						make_png(window, graph, "browse/out" + to_string(sn.index) + hc + ".png", v[v.size() / 2], 160, 120, heading);
				}

				html << "</table></td>";
			}

			html
				<< "</tr></table>"
				<< "<hr><h2>Neighbourhood</h2>"
				<< "<p>Positions up to two transitions away (clickable)</p>"
				<< svg << "</body></html>";
		}
	}
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		Graph const graph = loadGraph(config->db);

		if (!glfwInit()) error("could not initialize GLFW");

		// glfwWindowHint(GLFW_VISIBLE, GL_FALSE); // TODO: figure out how to make this work

		GLFWwindow * const window = glfwCreateWindow(640, 480, "Jiu Jitsu Map", nullptr, nullptr);

		if (!window) error("could not create window");

		glfwMakeContextCurrent(window);

		write_index(graph);
		write_todo(graph);

		foreach(tag : tags(graph)) write_tag_page(graph, tag, config->nogifs);

		if (config->node)
			write_position_page(window, graph, *config->node, config->nogifs);
		else
			foreach(n : nodenums(graph))
				write_position_page(window, graph, n, config->nogifs);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
