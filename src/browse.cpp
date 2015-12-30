
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

	struct Config
	{
		string db;
	};

	optional<Config> config_from_args(int const argc, char const * const * const argv)
	{
		namespace po = boost::program_options;

		po::options_description desc("options");
		desc.add_options()
			("help",
				"show this help")
			("db",
				po::value<string>()->default_value("GrappleMap.txt"),
				"database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) { std::cout << desc << '\n'; return none; }

		return Config{ vm["db"].as<string>() };
	}

	struct ImageMaker
	{
		GLFWwindow * const window;
		Graph const & graph;

		string png(
			string const output_dir,
			Position const pos,
			unsigned const heading,
			unsigned const width, unsigned const height) const
		{
			string const filename
				= to_string(boost::hash_value(pos))
				+ headings[heading]
				+ to_string(width) + 'x' + to_string(height)
				+ ".png";

			if (!boost::filesystem::exists(output_dir + filename))
			{
				double const ymax = std::max(.8, std::max(pos[0][Head].y, pos[1][Head].y));

				Camera camera;
				camera.hardSetOffset({0, ymax - 0.6, 0});
				camera.zoom(0.6);
				camera.rotateHorizontal(M_PI * 0.5 * heading);
				camera.rotateVertical((ymax - 0.6)/2);

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

				glfwSwapBuffers(window);

				boost::gil::rgb8_pixel_t buf[width * height];

				glFlush();
				glFinish();

				glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

				boost::gil::png_write_view(output_dir + filename,
					boost::gil::flipped_up_down_view(boost::gil::interleaved_view(width, height, buf, width*3)));
			}

			return filename;
		}

		string gif(
			string const output_dir,
			vector<Position> const & frames,
			unsigned const heading,
			unsigned const width, unsigned const height) const
		{
			string const filename
				= to_string(boost::hash_value(frames))
				+ to_string(width) + 'x' + to_string(height)
				+ headings[heading]
				+ ".gif";

			if (!boost::filesystem::exists(output_dir + filename))
			{
				vector<string> png_files;
				string const gif_frames_dir = output_dir + "gifframes/";
				foreach (pos : frames) png_files.push_back(png(gif_frames_dir, pos, heading, width, height));

				string command = "convert -depth 8 -delay 3 -loop 0 ";
				foreach (f : png_files) command += gif_frames_dir + f + ' ';
				command += output_dir + filename;

				std::system(command.c_str());
			}

			return filename;
		}

		string gifs(
			string const output_dir,
			vector<Position> const & frames,
			unsigned const width, unsigned const height) const
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
					foreach (pos : frames) png_files.push_back(png(gif_frames_dir, pos, heading, width, height));

					string command = "convert -depth 8 -delay 3 -loop 0 ";
					foreach (f : png_files) command += gif_frames_dir + f + ' ';
					command += output_dir + filename;

					std::system(command.c_str());
				}
			}

			return base_filename;
		}
	};


	vector<Position> frames_for_sequence(Graph const & graph, SeqNum const seqNum)
	{
		unsigned const frames_per_pos = 5;

		PositionInSequence location{seqNum, 0};

		vector<Position> r(10, graph[location]);

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
		"<script src='sorttable.js'></script>"
		"</head>";

	string nlspace(string const & s)
	{
		return replace_all(s, "\\n", " ");
	}

	string const output_dir = "GrappleMap/";

	void write_todo(Graph const & g)
	{
		std::ofstream html(output_dir + "todo.html");

		html << "</ul><h2>Dead ends</h2><ul>";

		foreach(n : nodenums(g))
			if (out(g, n).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul><h2>Dead starts</h2><ul>";

		foreach(n : nodenums(g))
			if (in(g, n).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul><h2>Untagged positions</h2><ul>";

		foreach(n : nodenums(g))
			if (tags_in_desc(g[n].description).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></li>";

		html << "</ul></body></html>";
	}

	void write_index(Graph const & g)
	{
		std::ofstream html(output_dir + "index.html");

		html
			<< html5head
			<< "<body style='vertical-align:top;text-align:center'>"
			<< "<h1><a href='https://github.com/Eelis/GrappleMap/blob/master/doc/FAQ.md'>GrappleMap</a></h1>"
			<< "<video autoplay loop><source src='random.mp4' type='video/mp4'></video>"
			<< "<br><br><h1>Index</h1><hr>"
			<< "<table><tr><td style='vertical-align:top;padding:50px'>"
			<< "<h2>Tags (" << tags(g).size() << ")</h2>"
			<< "<table style='display:inline-block;border:solid 1px' class='sortable'>"
			<< "<tr><th>Tag</th><th>Positions</th><th>Transitions</th></tr>";

		foreach(tag : tags(g))
		{
			set<NodeNum> s;
			foreach(n : tagged_nodes(g, tag)) s.insert(n);
				// i can't figure out how to get the size of the tagged_nodes result directly

			set<SeqNum> t;
			foreach(n : tagged_sequences(g, tag)) t.insert(n);

			html
				<< "<tr>"
				<< "<td><a href='tag-" << tag << ".html'>" << tag << "</a></td>"
				<< "<td>" << s.size() << "</td>"
				<< "<td>" << t.size() << "</td>"
				<< "</tr>";
		}

		html
			<< "</table></td><td style='padding:50px'>"
			<< "<h2>Positions (" << g.num_nodes() << ")</h2>"
			<< "<table style='display:inline-block;border: solid 1px' class='sortable'>"
			<< "<tr><th>Name</th><th>Incoming</th><th>Outgoing</th><th>Tags</th></tr>";


		foreach(n : nodenums(g))
			html
				<< "<tr>"
				<< "<td><a href='p" << n.index << "n.html'>" << nlspace(desc(g, n)) << "</a></td>"
				<< "<td>" << in(g, n).size() << "</td>"
				<< "<td>" << out(g, n).size() << "</td>"
				<< "<td>" << tags_in_desc(g[n].description).size() << "</td>"
				<< "</tr>";

		html
			<< "</table></td></tr></table>"
			<< "<h2>Transitions (" << g.num_sequences() << ")</h2>";

		html << "<table style='border: solid 1px' class='sortable'><tr><th>From</th><th>Via</th><th>To</th><th>Frames</th></tr>";

		foreach(s : seqnums(g))
		{
			html
				<< "<tr>"
				<< "<td><a href='p" << g.from(s).node.index << "n.html'>" << nlspace(desc(g, g.from(s).node)) << "</a></td>"
				<< "<td><b>" << nlspace(g[s].description.front()) << "</b></td>"
				<< "<td><a href='p" << g.to(s).node.index << "n.html'>" << nlspace(desc(g, g.to(s).node)) << "</a></td>"
				<< "<td>" << g[s].positions.size() << "</td>"
				<< "</tr>";
		}

		html << "</table>";

		html << "</body></html>";
	}

	string make_svg(Graph const & g, map<NodeNum, bool> const & nodes)
	{
		std::ostringstream dotstream;
		todot(g, dotstream, nodes);
		string const dot = dotstream.str();

		string const
			dotpath = output_dir + "tmp.dot",
			svgpath = output_dir + to_string(boost::hash_value(dot)) + ".svg";

		if (!boost::filesystem::exists(svgpath))
		{
			{ std::ofstream dotfile(dotpath); dotfile << dot; }
			auto cmd = "dot -Tsvg " + dotpath + " -o" + svgpath;
			std::system(cmd.c_str());
		}

		std::ifstream svgfile(svgpath);
		std::istreambuf_iterator<char> i(svgfile), e;
		string const r(i, e);

		return r.substr(r.find("<svg"));
	}

	string transition_image_title(Graph const & g, SeqNum const sn)
	{
		string r = "transition " + to_string( sn.index);
						
		if (auto ln = g[sn].line_nr) r += " @ line " + to_string(*ln);
		auto d = g[sn].description;
		d.erase(d.begin());
		foreach (l : d) r += '\n' + replace_all(l, "'", "&#39;");

		return r;
	}

	void write_tag_page(ImageMaker const mkimg, Graph const & g, string const & tag)
	{
		cout << '.' << std::flush;

		std::ofstream html(output_dir + "tag-" + tag + ".html");

		html << html5head << "<body style='text-align:center'><h1>Tag: " << tag << "</h1>";
		html << "<a href='index.html'>Index</a>";

		// positions

		set<NodeNum> nodes;
		foreach(n : tagged_nodes(g, tag))
			nodes.insert(n);

		if (!nodes.empty())
		{
			html << "<hr><h2>Positions</h2><p>Positions tagged '" << tag << "'</p>";

			foreach(n : nodes)
				html
					<< "<div style='display:inline-block;text-align:center'>"
					<< "<a href='p" << n.index << "e.html'>"
					<< nlspace(desc(g, n)) << "<br>"
					<< "<img alt='' title='" << n.index << "' src='" << mkimg.png(output_dir, g[n].position, 0, 480, 360) << "'>"
					<< "</a></div>";

			html << "<br>";
		}

		map<NodeNum, bool> m;
		foreach(n : nodes) m[n] = true;
		foreach(n : nodes_around(g, nodes)) m[n] = false;

		// transitions

		set<SeqNum> seqs;
		foreach(sn : tagged_sequences(g, tag))
			seqs.insert(sn);

		if (!seqs.empty())
		{
			html << "<hr><h2>Transitions</h2><p>Transitions that either go from one position tagged '" << tag << "' to another, or are themselves tagged '" << tag << "'</p><table style='margin:0px auto'>\n";

			foreach(sn : seqs)
				if (is_tagged(g, tag, sn))
				{
					auto const
						from = g.from(sn).node,
						to = g.to(sn).node;

					html
						<< "<tr>"
						<<   "<td style='text-align:right'><em>from</em> "
						<<      "<a href='p" << from.index << "w.html'>" << nlspace(desc(g, from)) << "</a> <em>via</em>"
						<<   "</td>"
						<<   "<td><div style='display:inline-block'>" << desc(g, sn)
						<<     "<img alt=''"
						<<     " src='" << mkimg.gif(output_dir, frames_for_sequence(g, sn), 0, 160, 120) << "'"
						<<     " title='" << transition_image_title(g, sn) << "'>"
						<<     "</div>"
						<<   "</td>"
						<<   "<td style='text-align:left'><em>to</em> "
						<<     "<a href='p" << to.index << "w.html'>" << nlspace(desc(g, to)) << "</a>"
						<<   "</td>"
						<< "</tr>";
				}

			html << "</table>";
		}

		html
			<< "<hr><h2>Neighbourhood</h2>"
			<< "<p>Positions up to one transition away from position";

		if (nodes.size() > 1) html << 's';

		html << " tagged '" << tag << "' (clickable)</p>" << make_svg(g, m) << "</body></html>";
	}

	void write_position_page(
		ImageMaker const mkimg,
		Graph const & graph,
		NodeNum const n)
	{
		string const pname = "p" + to_string(n.index);

		cout << '.' << std::flush;

		set<NodeNum> nodes{n};

		map<NodeNum, bool> m;
		m[n] = true;
		foreach(nn : nodes_around(graph, nodes, 2)) m[nn] = false;

		string const svg = make_svg(graph, m);

		struct Trans { SeqNum seq; vector<Position> frames; string base_filename; };

		auto const pos = graph[n].position;
		V2 const center = xz(pos[0][Core] + pos[1][Core]) / 2;

		PositionReorientation reo;
		reo.reorientation.offset.x = -center.x;
		reo.reorientation.offset.z = -center.y;

		vector<Trans> incoming, outgoing;

		size_t longest_in = 0, longest_out = 0;

		foreach (sn : in(graph, n))
		{
			auto v = frames_for_sequence(graph, sn);
			foreach (p : v) p = reo(inverse(graph.to(sn).reorientation)(p));
			assert(basicallySame(v.back(), reo(pos)));

			incoming.push_back({sn, v});
			longest_in = std::max(longest_in, v.size());
		}

		foreach (trans : incoming)
		{
			auto const p = trans.frames.front();
			trans.frames.insert(trans.frames.begin(), longest_in - trans.frames.size(), p);
			trans.base_filename = mkimg.gifs(output_dir, trans.frames, 160, 120);
		}

		foreach (sn : out(graph, n))
		{
			auto v = frames_for_sequence(graph, sn);
			foreach (p : v) p = reo(inverse(graph.from(sn).reorientation)(p));
			assert(basicallySame(v.front(), reo(pos)));

			outgoing.push_back({sn, v});
			longest_out = std::max(longest_out, v.size());
		}

		foreach (trans : outgoing)
		{
			auto const p = trans.frames.back();
			trans.frames.insert(trans.frames.end(), longest_out - trans.frames.size(), p);
			trans.base_filename = mkimg.gifs(output_dir, trans.frames, 160, 120);
		}

		for (unsigned heading = 0; heading != 4; ++heading)
		{
			char const hc = headings[heading];

			std::ofstream html(output_dir + pname + hc + ".html");

			html << html5head << "<body style='text-align:center'><table style='margin:0px auto'><tr>";

			if (!incoming.empty())
			{
				html << "<td style='text-align:center;vertical-align:top'><b>Incoming transitions</b><table>\n";

				foreach (trans : incoming)
				{
					auto sn = trans.seq;
					auto const from = graph.from(sn).node;

					html
						<< "<tr><td><hr>"
						<< "<em>from</em> <div style='display:inline-block'>"
						<< "<a href='p" << from.index << hc << ".html'>"
						<< replace_all(desc(graph, from), "\\n", "<br>") << "<br>"
						<< "<img alt='' title='" << from.index << "'"
						<< " src='" << mkimg.png(output_dir, trans.frames.front(), heading, 160, 120) << "'>"
						<< "</a></div> <em>via</em> <div style='display:inline-block'>"
						<< desc(graph, sn)
						<< "<img alt='' src='" << trans.base_filename << hc << ".gif'"
						<< " title='" << transition_image_title(graph, sn) << "'>"
						<< "</div> <em>to</em></td></tr>";
				}

				html << "</table></td>";
			}

			char const next_heading = headings[(heading + 1) % 4];
			char const prev_heading = headings[(heading + 3) % 4];

			html
				<< "<td style='text-align:center;vertical-align:top'><h3>Position:</h3><h1>"
				<< replace_all(desc(graph, n), "\\n", "<br>") << "<br><br>"
				<< "<img alt='' title='" << n.index << "' src='" << mkimg.png(output_dir, reo(pos), heading, 480, 360) << "'><br>"
				<< "<a href='" << pname << prev_heading << ".html'>↻</a> "
				<< "<a href='" << pname << next_heading << ".html'>↺</a> "
				<< "</h1>";
			
			auto const t = tags_in_desc(graph[n].description);
			if (!t.empty())
			{
				html << "Tags:";

				foreach(tag : t)
					html << " <a href='tag-" << tag << ".html'>" << tag << "</a>";

				html << "<br><br>";
			}

			html << "(<a href='index.html'>Index</a>)</td>";

			if (!outgoing.empty())
			{
				html << "<td style='text-align:center;vertical-align:top'><b>Outgoing transitions</b><table>";

				foreach (trans : outgoing)
				{
					auto const sn = trans.seq;
					auto const to = graph.to(sn).node;

					html
						<< "<tr><td><hr>"
						<< "<em>via</em> <div style='display:inline-block'>"
						<< desc(graph, sn)
						<< "<img alt='' src='" << trans.base_filename << hc << ".gif'"
						<< " title='" << transition_image_title(graph, sn) << "'>"
						<< "</div> <em>to</em> <div style='display:inline-block'>"
						<< "<a href='p" << to.index << hc << ".html'>"
						<< replace_all(desc(graph, to), "\\n", "<br>") << "<br>"
						<< "<img alt='' title='" << to.index << "'"
						<< " src='" << mkimg.png(output_dir, trans.frames.back(), heading, 160, 120) << "'>"
						<< "</a></div></td></tr>";
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

		ImageMaker const mkimg{ window, graph };

		foreach (t : tags(graph)) write_tag_page(mkimg, graph, t);

		foreach (n : nodenums(graph)) write_position_page(mkimg, graph, n);

		std::endl(cout);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
