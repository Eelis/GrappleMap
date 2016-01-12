#include "util.hpp"
#include "headings.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "images.hpp"
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
	
	vector<Position> frames_for_step(Graph const & graph, Step const step)
	{
		vector<Position> v = frames_for_sequence(graph, step.seq);
		if (step.reverse) std::reverse(v.begin(), v.end());
		return v;
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

	string html5head(string const title)
	{
		return
			"<!DOCTYPE html>"
			"<html lang='en'>"
			"<head><title>" + title + "</title>"
			"<meta charset='UTF-8'/>"
			"<script src='sorttable.js'></script>"
			"</head>";
	}

	string nlspace(string const & s) { return replace_all(s, "\\n", " "); }
	string nlbr(string const & s) { return replace_all(s, "\\n", "<br>"); }

	string const output_dir = "GrappleMap/";

	void write_todo(Graph const & g)
	{
		ofstream html(output_dir + "todo.html");

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
		ofstream html(output_dir + "index.html");

		html
			<< html5head("GrappleMap: Index")
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
				<< "<td><a href='tag-" << tag << "-w.html'>" << tag << "</a></td>"
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

	string make_svg(Graph const & g, map<NodeNum, bool> const & nodes, char const heading)
	{
		std::ostringstream dotstream;
		todot(g, dotstream, nodes, heading);
		string const dot = dotstream.str();

		string const
			dotpath = output_dir + "tmp.dot",
			svgpath = output_dir + to_string(boost::hash_value(dot)) + ".svg";

		if (!boost::filesystem::exists(svgpath))
		{
			{ std::ofstream dotfile(dotpath); dotfile << dot; }
			auto const cmd = "dot -Tsvg " + dotpath + " -o" + svgpath;
			if (std::system(cmd.c_str()) != 0)
				throw runtime_error("dot fail");
		}

		std::ifstream svgfile(svgpath);
		std::istreambuf_iterator<char> i(svgfile), e;
		string const r(i, e);

		auto svgpos = r.find("<svg");

		if (svgpos == string::npos) throw runtime_error("svg fail");

		return r.substr(svgpos);
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

	ImageMaker::BgColor bg_color(bool const top, bool const bottom)
	{
		if (top) return ImageMaker::RedBg;
		if (bottom) return ImageMaker::BlueBg;
		return ImageMaker::WhiteBg;
	}

	vector<Position> smoothen(vector<Position> v)
	{
		Position last_pos = v[0];

		foreach (p : v)
		foreach (j : playerJoints)
			p[j] = last_pos[j] = last_pos[j] * 0.6 + p[j] * 0.4;

		return v;
	}

	string transition_gif(
		ImageMaker const & mkimg,
		string const output_dir,
		vector<Position> frames,
		MirroredHeading const h,
		ImageMaker::BgColor const bg_color)
	{
		return mkimg.gif(output_dir, smoothen(frames), h, 160, 120, bg_color);
	}

	string transition_gifs(
		ImageMaker const & mkimg,
		string const output_dir,
		vector<Position> frames,
		ImageMaker::BgColor const bg_color)
	{
		return mkimg.gifs(output_dir, smoothen(frames), 160, 120, bg_color);
	}

	void write_tag_page(ImageMaker const mkimg, Graph const & g, string const & tag)
	{
		cout << '.' << std::flush;

		set<NodeNum> nodes;
		foreach(n : tagged_nodes(g, tag))
			nodes.insert(n);


		struct Trans { SeqNum seq; bool top, bottom; };

		vector<Trans> seqs;
		foreach(sn : tagged_sequences(g, tag))
		{
			auto const props = properties(g, sn);

			seqs.push_back(Trans{sn, props.count("top") != 0, props.count("bottom") != 0});
		}

		{
			auto i = std::partition(seqs.begin(), seqs.end(),
				[](Trans const & t) { return t.top; });
			std::partition(i, seqs.end(),
				[](Trans const & t) { return !t.bottom; });
		}

		map<NodeNum, bool> m;
		foreach(n : nodes) m[n] = true;
		foreach(n : nodes_around(g, nodes)) m[n] = false;

		string const base_filename = "tag-" + tag + '-';

		foreach (h : headings())
		{
			string const neighbourhood = make_svg(g, m, code(h));

			ofstream html(output_dir + base_filename + code(h) + ".html");

			html
				<< html5head("tag: " + tag)
				<< "<body style='text-align:center'>"
				<< "<h1>Tag: " << tag << "<br><br>"
				<< "<a href='" << base_filename << code(rotate_left(h)) << ".html'>↻</a> "
				<< "<a href='" << base_filename << code(mirror(h)) << ".html'>⇄</a> "
				<< "<a href='" << base_filename << code(rotate_right(h)) << ".html'>↺</a>"
				<< "</h1>"
				<< "<a href='index.html'>(index)</a>";

			if (!nodes.empty())
			{
				html << "<hr><h2>Positions</h2><p>Positions tagged '" << tag << "'</p>";

				foreach(n : nodes)
				{
					html
						<< "<div style='display:inline-block;text-align:center'>"
						<< "<a href='p" << n.index << code(h) << ".html'>"
						<< nlspace(desc(g, n)) << "<br>"
						<< "<img alt='' title='" << n.index << "' src='"
						<< mkimg.png(output_dir, orient_canonically(g[n].position), h, 480, 360, ImageMaker::WhiteBg) << "'>"
						<< "</a></div>";
				}

				html << "<br>";
			}

			if (!seqs.empty())
			{
				html
					<< "<hr><h2>Transitions</h2><p>Transitions that either go from one position tagged '"
					<< tag << "' to another, or are themselves tagged '"
					<< tag << "'</p><table style='margin:0px auto'>";

				foreach(trans : seqs)
					if (is_tagged(g, tag, trans.seq))
					{
						auto const
							from = g.from(trans.seq).node,
							to = g.to(trans.seq).node;

						html
							<< "<tr>"
							<<   "<td style='text-align:right'><em>from</em> "
							<<      "<a href='p" << from.index << "w.html'>" << nlspace(desc(g, from)) << "</a> <em>via</em>"
							<<   "</td>"
							<<   "<td";

						if (trans.top) html << " style='background:#ffe0e0'";
						else if (trans.bottom) html << " style='background:#e0e0ff'";

						auto frames = frames_for_sequence(g, trans.seq);

						if (g.from(trans.seq).reorientation.swap_players)
							foreach (p : frames)  std::swap(p[0], p[1]);

						auto const reo = canonical_reorientation(frames.front());

						foreach (p : frames)  p = reo(p);

						html
							<< "><div style='display:inline-block'>" << desc(g, trans.seq)
							<<     "<img alt=''"
							<<     " src='" << transition_gif(mkimg, output_dir, frames, h, bg_color(trans.top, trans.bottom)) << "'"
							<<     " title='" << transition_image_title(g, trans.seq) << "'>"
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

			html << " tagged '" << tag << "' (clickable)</p>" << neighbourhood << "</body></html>";
		}
	}

	namespace position_page
	{
		struct Trans
		{
			Step step;
			bool top, bottom;
			vector<Position> frames;
			string base_filename;
		};

		ImageMaker::BgColor bg_color(Trans const & t) { return ::bg_color(t.top, t.bottom); }

		struct Context
		{
			ImageMaker mkimg;
			std::ostream & html;
			Graph const & graph;
			NodeNum n;
			vector<Trans> incoming, outgoing;
			MirroredHeading heading;
		};

		void write_incoming(Context const & ctx)
		{
			char const hc = code(ctx.heading);

			if (!ctx.incoming.empty())
			{
				ctx.html << "<td style='text-align:center;vertical-align:top'><b>Incoming transitions</b><table>\n";

				foreach (trans : ctx.incoming)
				{
					auto const from_node = from(ctx.graph, trans.step).node;
					auto c = bg_color(trans);

					auto rotate_pos = orient_canonically(trans.frames.front(), true);
					if (ctx.heading.mirror) rotate_pos = mirror(rotate_pos);

					ctx.html
						<< "<tr><td style='" << ImageMaker::css(c) << "'>"
						<< "<em>from</em> <div style='display:inline-block'>"
						<< "<a href='p" << from_node.index << hc << ".html'>"
						<< nlbr(desc(ctx.graph, from_node)) << "<br>"
						<< "<img alt='' title='" << from_node.index << "'"
						<< " src='"
						<< ctx.mkimg.rotation_gif(output_dir, rotate_pos, 160, 120, c)
						<< "'></a></div> <em>via</em> <div style='display:inline-block'>"
						<< desc(ctx.graph, trans.step.seq)
						<< "<img alt='' src='" << trans.base_filename << hc << ".gif'"
						<< " title='" << transition_image_title(ctx.graph, trans.step.seq) << "'>"
						<< "</div> <em>to</em></td></tr>";
				}

				ctx.html << "</table></td>";
			}
		}

		void write_outgoing(Context const & ctx)
		{
			char const hc = code(ctx.heading);

			if (!ctx.outgoing.empty())
			{
				ctx.html << "<td style='text-align:center;vertical-align:top'><b>Outgoing transitions</b><table>";

				foreach (trans : ctx.outgoing)
				{
					auto const to_node = to(ctx.graph, trans.step).node;
					auto c = bg_color(trans);

					auto rotate_pos = orient_canonically(trans.frames.back(), true);
					if (ctx.heading.mirror) rotate_pos = mirror(rotate_pos);

					ctx.html
						<< "<tr><td style='" << ImageMaker::css(c) << "'>"
						<< "<em>via</em> <div style='display:inline-block'>"
						<< desc(ctx.graph, trans.step.seq)
						<< "<img alt='' src='" << trans.base_filename << hc << ".gif'"
						<< " title='" << transition_image_title(ctx.graph, trans.step.seq) << "'>"
						<< "</div> <em>to</em> <div style='display:inline-block'>"
						<< "<a href='p" << to_node.index << hc << ".html'>"
						<< nlbr(desc(ctx.graph, to_node)) << "<br>"
						<< "<img alt='' title='" << to_node.index << "'"
						<< " src='"
						<< ctx.mkimg.rotation_gif(output_dir, rotate_pos, 160, 120, bg_color(trans))
						<< "'>"
						<< "</a></div></td></tr>";
				}

				ctx.html << "</table></td>";
			}
		}

		void write_it(ImageMaker const mkimg, Graph const & graph, NodeNum const n)
		{
			string const pname = "p" + to_string(n.index);

			cout << ' ' << n.index << std::flush;

			set<NodeNum> nodes{n};

			map<NodeNum, bool> m;
			m[n] = true;
			foreach(nn : nodes_around(graph, nodes, 2)) m[nn] = false;

			auto const pos = graph[n].position;

			PositionReorientation const reo = canonical_reorientation(pos);

			vector<Trans> incoming, outgoing;

			size_t longest_in = 0, longest_out = 0;

			foreach (step : in_steps(graph, n))
			{
				auto v = frames_for_step(graph, step);
				auto const this_side = step.reverse ? graph.from(step.seq) : graph.to(step.seq);

				foreach (p : v) p = reo(inverse(this_side.reorientation)(p));
				assert(basicallySame(v.back(), reo(pos)));

				auto const props = properties(graph, step.seq);

				bool top = props.count("top")!=0;
				bool bottom = props.count("bottom")!=0;

				bool const sweep = is_sweep(graph, step.seq);

				if (top && sweep)
				{ top = false; bottom = true; }
				else if (bottom && sweep)
				{ bottom = false; top = true; }

				incoming.push_back({step, top, bottom, v, {}});

				longest_in = std::max(longest_in, v.size());
			}

			foreach (trans : incoming)
			{
				auto const p = trans.frames.front();
				trans.frames.insert(trans.frames.begin(), longest_in - trans.frames.size(), p);
				trans.base_filename = transition_gifs(mkimg, output_dir, trans.frames, bg_color(trans));
			}

			foreach (step : out_steps(graph, n))
			{
				auto v = frames_for_step(graph, step);
				auto const this_side = step.reverse ? graph.to(step.seq) : graph.from(step.seq);

				foreach (p : v) p = reo(inverse(this_side.reorientation)(p));
				assert(basicallySame(v.front(), reo(pos)));

				auto const props = properties(graph, step.seq);

				outgoing.push_back({step, props.count("top")!=0, props.count("bottom")!=0, v, {}});
				longest_out = std::max(longest_out, v.size());
			}

			foreach (trans : outgoing)
			{
				auto const p = trans.frames.back();
				trans.frames.insert(trans.frames.end(), longest_out - trans.frames.size(), p);
				trans.base_filename = transition_gifs(mkimg, output_dir, trans.frames, bg_color(trans));
			}

			auto order_transitions = [](vector<Trans> & v)
				{
					auto i = std::partition(v.begin(), v.end(),
						[](Trans const & t) { return t.top; });

					std::partition(i, v.end(),
						[](Trans const & t) { return !t.bottom; });
				};

			order_transitions(incoming);
			order_transitions(outgoing);

			foreach (h : headings())
			{
				char const hc = code(h);

				ofstream html(output_dir + pname + hc + ".html");

				html << html5head("position: " + nlspace(desc(graph, n))) << "<body style='text-align:center'><table style='margin:0px auto'><tr>";

				Context ctx{mkimg, html, graph, n, incoming, outgoing, h};

				write_incoming(ctx);

				html
					<< "<td style='text-align:center;vertical-align:top'><h3>Position:</h3><h1>"
					<< nlbr(desc(graph, n)) << "<br><br>"
					<< "<img alt='' title='" << n.index << "' src='"
					<< mkimg.png(output_dir, reo(pos), h, 480, 360, mkimg.WhiteBg) << "'><br>"
					<< "<a href='" << pname << code(rotate_left(h)) << ".html'>↻</a> "
					<< "<a href='" << pname << code(mirror(h)) << ".html'>⇄</a> "
					<< "<a href='" << pname << code(rotate_right(h)) << ".html'>↺</a> "
					<< "</h1>";
				
				auto const t = tags_in_desc(graph[n].description);
				if (!t.empty())
				{
					html << "Tags:";

					foreach(tag : t)
						html << " <a href='tag-" << tag << "-" << hc << ".html'>" << tag << "</a>";

					html << "<br><br>";
				}

				html << "(<a href='index.html'>Index</a>)</td>";

				write_outgoing(ctx);

				html
					<< "</tr></table>"
					<< "<hr><h2>Neighbourhood</h2>"
					<< "<p>Positions up to two transitions away (clickable)</p>"
					<< make_svg(graph, m, hc) << "</body></html>";
			}
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

		foreach (n : nodenums(graph)) position_page::write_it(mkimg, graph, n);

		std::endl(cout);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
