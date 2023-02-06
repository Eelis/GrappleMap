#define BOOST_NO_CXX11_SCOPED_ENUMS
	// see https://www.robertnitsch.de/notes/cpp/cpp11_boost_filesystem_undefined_reference_copy_file

#include "png.h"
#include "util.hpp"
#include "headings.hpp"
#include "camera.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "images.hpp"
#include "metadata.hpp"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <signal.h>

using namespace GrappleMap;

inline std::size_t hash_value(V3 const v) // todo: put elsewhere
{
	size_t seed = 0;
	boost::hash_combine(seed, v.x);
	boost::hash_combine(seed, v.y);
	boost::hash_combine(seed, v.z);
	return seed;
}

#include <boost/functional/hash.hpp>

#ifndef int_p_NULL // ffs
	#define int_p_NULL (int*)NULL // https://github.com/ignf/gilviewer/issues/8
#endif


#include <boost/version.hpp>
#if (BOOST_VERSION < 106800)
#   include <boost/gil/gil_all.hpp>
# 	include <boost/gil/extension/io/png_io.hpp>
#		include <boost/gil/extension/io/png_dynamic_io.hpp>
#else
#   include <boost/gil.hpp>
#		include <boost/gil/extension/io/png.hpp>
#endif

namespace
{
	volatile bool keep_running = true;

	string img(string title, string src, string alt)
	{
		return "<img alt='" + alt + "' src='" + src + "' title='" + title + "'>";
	}

	string vid(string title, int width, int height, string src)
	{
		return "<video title='" + title + "' autoplay='autoplay' loop='loop' width='" + to_string(width) + "' height='" + to_string(height) + "'>"
			+ "<source src='" + src + "' type='video/mp4'/>"
			+ "</video>";
		// todo: escape title
	}

	string link(string href, string content)
	{
		return "<a href='" + href + "'>" + content + "</a>";
	}

	string div(string style, string content)
	{
		return "<div style='" + style + "'>" + content + "</div>";
	}

	struct Config
	{
		string db;
		string output_dir;
		optional<string> image_url;
		bool no_anim;
	};

	template<typename T>
	optional<T> opt_arg(boost::program_options::variables_map const & vm, string const & name)
	{
		if (vm.count(name)) return vm[name].as<T>();
		return optional<T>();
	}

	optional<Config> config_from_args(int const argc, char const * const * const argv)
	{
		namespace po = boost::program_options;

		po::options_description desc("Options");
		desc.add_options()
			("help,h",
				"show this help")
			("output_dir",
				po::value<string>()->default_value("./GrappleMap"),
				"output directory")
			("image_url",
				po::value<string>())
			("no_anim",
				po::value<bool>()->default_value(false))
			("db",
				po::value<string>()->default_value("GrappleMap.txt"),
				"database file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			cout << desc <<
				"\nWarning: The first run will take many hours.\n\n"
				"In subsequent runs, pictures are only (re)created "
				"for new or changed transitions.\n";

			return none;
		}

		return Config
			{ vm["db"].as<string>()
			, vm["output_dir"].as<string>()
			, opt_arg<string>(vm, "image_url")
			, vm["no_anim"].as<bool>() };
	}

	vector<Position> frames_for_sequence(Graph const & graph, SeqNum const seqNum)
	{
		unsigned const frames_per_pos = graph[seqNum].detailed ? 3 : 5;

		PositionInSequence location{seqNum, 0};

		vector<Position> r(10, at(location, graph));

		for (; next(location, graph); location = *next(location, graph))
			for (unsigned howfar = 0; howfar != frames_per_pos; ++howfar)
				r.push_back(between(
					at(location, graph),
					at(*next(location, graph), graph),
					howfar / double(frames_per_pos)));

		r.resize(r.size() + 10, graph[seqNum].positions.back());

		return r;
	}
	
	vector<Position> frames_for_step(Graph const & graph, Step const step)
	{
		vector<Position> v = frames_for_sequence(graph, *step);
		if (step.reverse) std::reverse(v.begin(), v.end());
		return v;
	}

	string desc(Graph::Node const & n)
	{
		auto desc = n.description;
		return desc.empty() ? "?" : desc.front();
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
			"<style>a{text-decoration:none}</style>"
			"</head>";
	}

	string nlspace(string const & s) { return replace_all(s, "\\n", " "); }
	string nlbr(string const & s) { return replace_all(s, "\\n", "<br>"); }

	void write_todo(Graph const & g, string const output_dir)
	{
		ofstream html(output_dir + "/todo.html");

		html << "</ul><h2>Dead ends</h2><ul>";

		foreach(n : nodenums(g))
			if (g[n].out.empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g[n])) << "</a></li>";

		html << "</ul><h2>Dead starts</h2><ul>";

		foreach(n : nodenums(g))
			if (g[n].in.empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g[n])) << "</a></li>";

		html << "</ul><h2>Untagged positions</h2><ul>";

		foreach(n : nodenums(g))
			if (tags(g[n]).empty())
				html << "<li><a href='p" << n.index << "n.html'>" << nlspace(desc(g[n])) << "</a></li>";

		html << "</ul></body></html>";
	}

	void write_lists(Graph const & g, string const output_dir)
	{
		ofstream html(output_dir + "/lists.html");

		html
			<< html5head("GrappleMap: Index")
			<< "<body style='vertical-align:top;text-align:center'>"
			<< "<h1><a href='https://github.com/Eelis/GrappleMap/'>GrappleMap</a></h1>"
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
				<< "<td><a href='index.html?" << tag << "'>" << tag << "</a></td>"
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
				<< "<td><a href='position/" << n.index << "n.html'>" << nlspace(desc(g[n])) << "</a></td>"
				<< "<td>" << g[n].in.size() << "</td>"
				<< "<td>" << g[n].out.size() << "</td>"
				<< "<td>" << tags(g[n]).size() << "</td>"
				<< "</tr>";

		html
			<< "</table></td></tr></table>"
			<< "<h2>Transitions (" << g.num_sequences() << ")</h2>";

		html << "<table style='border: solid 1px' class='sortable'><tr><th>From</th><th>Via</th><th>To</th><th>Frames</th><th>Tags</th></tr>";

		foreach(s : seqnums(g))
		{
			set<string> tt = tags(g[s]);

			{
				set<string> const from_tags = tags(g[*g[s].from]);
				foreach(t : tags(g[*g[s].to]))
					if (from_tags.find(t) != from_tags.end())
						tt.insert(t);
			}

			html
				<< "<tr>"
				<< "<td><a href='p" << g[s].from->index << "n.html'>" << nlspace(desc(g[*g[s].from])) << "</a></td>"
				<< "<td><b>" << nlspace(g[s].description.front()) << "</b></td>"
				<< "<td><a href='p" << g[s].to->index << "n.html'>" << nlspace(desc(g[*g[s].to])) << "</a></td>"
				<< "<td>" << g[s].positions.size() << "</td>"
				<< "<td>" << tt.size() << "</td>"
				<< "</tr>";
		}

		html << "</table>";

		html << "</body></html>";
	}

	string transition_image_title(Graph const & g, SeqNum const sn)
	{
		string r;

		foreach (l : g[sn].description)
			r += replace_all(replace_all(l, "'", "&#39;"), "\\n", " ") + '\n';

		r += "(t" + to_string(sn.index);
		if (auto ln = g[sn].line_nr) r += '@' + to_string(*ln);
		r += ')';

		return r;
	}

	string position_image_title(Graph const & g, NodeNum const n)
	{
		string r;

		foreach (l : g[n].description)
			r += replace_all(replace_all(l, "'", "&#39;"), "\\n", " ") + '\n';

		r += "(p" + to_string(n.index);
		if (auto ln = g[n].line_nr) r += '@' + to_string(*ln);
		r += ')';

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
		{
			double const lag = std::min(0.55, 0.3 + p[j].y);
			p[j] = last_pos[j] = last_pos[j] * lag + p[j] * (1 - lag);
		}

		return v;
	}

	void transition_gif(
		ImageMaker & mkimg,
		vector<Position> frames,
		ImageView const v,
		ImageMaker::BgColor const bg_color,
		string const base_linkname)
	{
		mkimg.gif(smoothen(frames), v, 200, 150, bg_color, base_linkname);
	}

	string transition_gifs(
		ImageMaker & mkimg,
		vector<Position> frames,
		ImageMaker::BgColor const bg_color,
		string const base_linkname)
	{
		return mkimg.gifs(smoothen(frames), 200, 150, bg_color, base_linkname);
	}

	ImageView xmirror(ImageView const v)
	{
		assert(v.heading);

		return
			{ !v.mirror
			, *v.heading == Heading::W || *v.heading == Heading::E
				? opposite(*v.heading)
				: *v.heading
			, {}};
	}

	ImageView zmirror(ImageView const v)
	{
		assert(v.heading);

		return
			{ !v.mirror
			, *v.heading == Heading::N || *v.heading == Heading::S
				? opposite(*v.heading)
				: *v.heading
			, {}};
	}

	void write_view_controls(std::ostream & html, ImageView const view, string const base)
	{
		html << "View: ";

		if (view.heading)
		{
			html
				<< link(base + code(ImageView{view.mirror, rotate_left(*view.heading), {}}) + ".html", "↻") << ' '
				<< link(base + code(ImageView{view.mirror, rotate_right(*view.heading), {}}) + ".html", "↺") << ", "
				<< link(base + code(xmirror(view)) + ".html", "⇄") << ' '
				<< link(base + code(zmirror(view)) + ".html", "⇅");
			/*
				<< ", "
				<< link(base + code(ImageView{view.mirror, {}, 0u}) + ".html", "<span style='color:red'>☻</span>")
				<< link(base + code(ImageView{view.mirror, {}, 1u}) + ".html", "<span style='color:blue'>☻</span>");
			*/
		}
		else if (view.player)
		{
			char const player_code[2] = {'t', 'b'};

			html
				<< link(base + player_code[1 - view.player->index] + ".html",
					string("<span style='color:") + (view.player->index == 0 ? "blue" : "red") + "'>☻</span>")
				<< ' '
				<< link(base + code(ImageView{!view.mirror, {}, view.player}) + ".html", "⇄")
				<< ' '
				<< link(base + code(ImageView{view.mirror, Heading::N, {}}) + ".html", "3<sup><span style='font-size:smaller'>rd</span></sup>");
		}
		else abort();
	}

	double range(Position const & p)
	{
		return distanceSquared(p[player0][Core], p[player1][Core]);
	}

	void write_transition_gifs(ImageMaker & mkimg, Graph const & g)
	{
		cout << "Writing " << g.num_sequences() << " * 8 standalone transition gifs...   0%";

		foreach (sn : seqnums(g))
		{
			if (!keep_running) break;

			progress(sn.index, g.num_sequences());

			auto const props = properties(g[sn]);

			bool top = elem("top", props);
			bool bottom = elem("bottom", props);

			auto frames = frames_for_sequence(g, sn);

			if (g[sn].from.reorientation.swap_players)
				foreach (p : frames) swap_players(p);

			auto const reo = canonical_reorientation_with_mirror(frames.front());

			foreach (p : frames) p = reo(p);

			foreach (v : views())
				transition_gif(
					mkimg, frames, v,
					bg_color(top, bottom),
					't' + to_string(sn.index));
		}

		std::endl(cout);
	}

	namespace position_page
	{
		struct Trans
		{
			Step step;
			bool top, bottom;
			vector<Position> frames;
			string base_filename;
			NodeNum other_node;
		};

		ImageMaker::BgColor bg_color(Trans const & t) { return ::bg_color(t.top, t.bottom); }
		
		struct Context
		{
			ImageMaker & mkimg;
			Graph const & graph;
			NodeNum n;
			vector<Trans> incoming, outgoing;
			ImageView view;
			string const image_url;
			TagQuery query;
		};

		string transition_card(Context const & ctx, Trans const & trans)
		{
			return "<em>via</em> "
				+ link(
					"../composer/index.html?" + to_string(trans.step->index),
					vid(
						transition_image_title(ctx.graph, *trans.step), 200, 150,
						ctx.image_url + "/" + trans.base_filename + code(ctx.view) + ".mp4"))
				+ " <em>to</em>";
		}

		void write_incoming(Context const & ctx, ostream & html)
		{
			if (ctx.incoming.empty()) return;

			html << "<td style='text-align:center;vertical-align:top'><b>Incoming transitions</b><table>\n";

			foreach (trans : ctx.incoming)
			{
				auto const v = is_sweep(ctx.graph, *trans.step) ? xmirror(ctx.view) : ctx.view;

				html
					<< "<tr><td style='" << ImageMaker::css(bg_color(trans)) << "'>"
					<< "<em>from</em> "
					<< div("display:inline-block",
						link('p' + to_string(trans.other_node.index) + code(v) + ".html",
							vid(position_image_title(ctx.graph, trans.other_node),
								200, 150,
								ctx.image_url + "/" + ctx.mkimg.rotation_gif(
									translateNormal(trans.frames.front()),
									ctx.view, 200, 150, bg_color(trans),
									"rot" + to_string(ctx.n.index)
									+ "in" + to_string(trans.step->index) + code(v)))))
					<< ' ' << transition_card(ctx, trans) << "</td></tr>";
			}

			html << "</table></td>";
		}

		void write_outgoing(Context const & ctx, ostream & html)
		{
			if (ctx.outgoing.empty()) return;

			html << "<td style='text-align:center;vertical-align:top'><b>Outgoing transitions</b><table>";

			foreach (trans : ctx.outgoing)
			{
				auto const v = is_sweep(ctx.graph, *trans.step) ? xmirror(ctx.view) : ctx.view;

				html
					<< "<tr><td style='" << ImageMaker::css(bg_color(trans)) << "'>"
					<< transition_card(ctx, trans)
					<< " <div style='display:inline-block'>"
					<< link(
						'p' + to_string(trans.other_node.index) + code(v) + ".html",
						vid(position_image_title(ctx.graph, trans.other_node), 200, 150,
							ctx.image_url + "/" + ctx.mkimg.rotation_gif(
								translateNormal(trans.frames.back()),
								ctx.view, 200, 150, bg_color(trans),
								"rot" + to_string(ctx.n.index)
								+ "out" + to_string(trans.step->index) + code(v))))
					<< "</a></div></td></tr>";
			}

			html << "</table></td>";
		}

		void write_tag_list(Context const & ctx, ostream & html)
		{
			html << "<br><br><table style='border-collapse:collapse;padding:0px;margin:auto'><tr><td style='vertical-align:top;padding:0px'>Tags:&nbsp;</td><td style='padding:0px;text-align:left'>";

			int i = 0;

			foreach (tag : tags(ctx.graph[ctx.n]))
			{
				if (i != 0)
				{
					html << ", ";
					if (i % 2 == 0) html << "<br>";
				}

				++i;

				html << link("../index.html?" + tag, tag);
			}

			html << "</td></tr></table>";
		}

		void write_nav_links(Context const & ctx, ostream & html)
		{
			vector<string> v;
			foreach(e : ctx.query)
			{
				v.push_back(e.first);
				if (!e.second) v.back() = '-' + v.back();
			}

			html
				<< "<br>Go to:"
				<< " " << link("../composer/index.html?p" + to_string(ctx.n.index), "composer")
				<< ", " << link("../explorer/index.html?" + to_string(ctx.n.index), "explorer")
				<< ", " << link("../index.html?" + join(v, ","), "search");
		}

		void write_heading(Context const & ctx, ostream & html)
		{
			auto const pos_to_show = orient_canonically_with_mirror(ctx.graph[ctx.n].position);

			double const ymax = std::max(.8, std::max(
				pos_to_show[player0][Head].y,
				pos_to_show[player1][Head].y));

			ctx.mkimg.png(pos_to_show, ymax, ctx.view,
				480, 360, ctx.mkimg.WhiteBg, 'p' + to_string(ctx.n.index));

			ctx.mkimg.png(pos_to_show, ymax, ctx.view,
				320, 240, ctx.mkimg.WhiteBg, 'p' + to_string(ctx.n.index));

			html
				<< "<h1><a href='https://github.com/Eelis/GrappleMap/'>GrappleMap</a></h1>"
				<< "<h1>" << nlbr(desc(ctx.graph[ctx.n])) << "</h1>"
				<< "<br><br>"
				<< img(position_image_title(ctx.graph, ctx.n),
					ctx.image_url + "/p" + to_string(ctx.n.index) + code(ctx.view) + "480x360.png"
					, "")
				<< "<br>";
		}

		void write_page(Context const & ctx)
		{
			map<NodeNum, bool> m;
			m[ctx.n] = true;
			foreach(nn : nodes_around(ctx.graph, set<NodeNum>{ctx.n}, 2, true /* no taps */))
				m[nn] = false;

			std::ostringstream dotstream;
			todot(ctx.graph, dotstream, m, code(ctx.view));

			string const
				dot = dotstream.str(),
				svg_filename = to_string(boost::hash_value(dot)) + ".svg",
				svg_linkname = "neighbourhood" + to_string(ctx.n.index) + code(ctx.view) + ".svg";

			ctx.mkimg.store(svg_filename, svg_linkname, [&]
				{
					ctx.mkimg.make_svg(svg_filename, dot);
				});

			std::ostringstream oss;
			oss
				<< html5head("position: " + nlspace(desc(ctx.graph[ctx.n])))
				<< "<body style='text-align:center'><table style='margin:0px auto'><tr>";

			write_incoming(ctx, oss);

			oss << "<td style='text-align:center;vertical-align:top'>";
			write_heading(ctx, oss);
			write_view_controls(oss, ctx.view, "p" + to_string(ctx.n.index));
			write_tag_list(ctx, oss);
			write_nav_links(ctx, oss);
			oss << "</td>";

			write_outgoing(ctx, oss);

			oss
				<< "</tr></table>"
				<< "<hr><h2>Neighbourhood</h2>"
				<< "<p>Positions up to two transitions away</p>"
				<< "<object data='" << svg_linkname << "' type='image/svg+xml'></object>"
				<< "</body></html>";

			string const
				html = oss.str(),
				html_filename = to_string(boost::hash_value(html)) + ".html",
				html_linkname = "p" + to_string(ctx.n.index) + code(ctx.view) + ".html";

			ctx.mkimg.store(html_filename, html_linkname, [&]
				{
					ofstream f(ctx.mkimg.res_dir + "/store/" + html_filename);
					f << html;
				});
		}

		void write_it(ImageMaker & mkimg, Graph const & graph, NodeNum const n, string const image_url)
		{
			set<NodeNum> nodes{n};

			auto const pos = graph[n].position;

			PositionReorientation const reo = canonical_reorientation_with_mirror(pos);
			assert(!reo.swap_players);

			vector<Trans> incoming, outgoing;

			size_t longest_in = 0, longest_out = 0;

			foreach (step : graph[n].in)
			{
				auto v = frames_for_step(graph, step);

				auto const this_side = to(step, graph);
				auto const other_side = from(step, graph);

				foreach (p : v) p = reo(inverse(this_side.reorientation)(p));
				assert(basicallySame(v.back(), reo(pos)));

				auto const props = properties(graph[*step]);

				bool top = elem("top", props);
				bool bottom = elem("bottom", props);

				bool const sweep = is_sweep(graph, *step);

				if (top && sweep)
				{ top = false; bottom = true; }
				else if (bottom && sweep)
				{ bottom = false; top = true; }

				incoming.push_back({step, top, bottom, v, {}, *other_side});

				longest_in = std::max(longest_in, v.size());
			}

			foreach (trans : incoming)
			{
				auto const p = trans.frames.front();
				trans.frames.insert(trans.frames.begin(), longest_in - trans.frames.size(), p);
				trans.base_filename = transition_gifs(
					mkimg, trans.frames, bg_color(trans),
					to_string(n.index) + "in" + to_string(trans.step->index));
			}

			foreach (step : graph[n].out)
			{
				auto v = frames_for_step(graph, step);

				auto const this_side = from(step, graph);
				auto const other_side = to(step, graph);

				foreach (p : v) p = reo(inverse(this_side.reorientation)(p));
				assert(basicallySame(v.front(), reo(pos)));

				auto const props = properties(graph[*step]);

				outgoing.push_back({step, elem("top", props), elem("bottom", props), v, {}, *other_side});
				longest_out = std::max(longest_out, v.size());
			}

			foreach (trans : outgoing)
			{
				auto const p = trans.frames.back();
				trans.frames.insert(trans.frames.end(), longest_out - trans.frames.size(), p);
				trans.base_filename = transition_gifs(
					mkimg, trans.frames, bg_color(trans),
					to_string(n.index) + "out" + to_string(trans.step->index));
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

			foreach (v : views())
			{
				if (!keep_running) break;

				write_page(Context
					{ mkimg, graph, n, incoming, outgoing
					, v, image_url, query_for(graph, n) });
			}
		}
	}
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		struct sigaction act;
		act.sa_handler = +[](int) {
				printf("\nExiting gracefully, please wait...\n");
				keep_running = false;
			};
		act.sa_mask = sigset_t{0};
		act.sa_flags = 0;

		sigaction(SIGINT, &act, nullptr);

		optional<Config> const config = config_from_args(argc, argv);
		if (!config) return 0;

		string const output_dir = config->output_dir;

		boost::filesystem::create_directory(config->output_dir + "/GrappleMap");
		boost::filesystem::create_directory(output_dir + "/position");
		boost::filesystem::create_directory(output_dir + "/graphs");
		boost::filesystem::create_directory(output_dir + "/res");
		boost::filesystem::create_directory(output_dir + "/res/store");

		Graph const graph = loadGraph(config->db);

		write_lists(graph, output_dir);
		write_todo(graph, output_dir);

		ImageMaker mkimg(graph, output_dir + "/res/");

		mkimg.no_anim = config->no_anim;

		ofstream(output_dir + "/config.js")
			<< "image_url='"
			<< (config->image_url
					? *(config->image_url)
					: "res/")
			<< "';";

		write_transition_gifs(mkimg, graph);

		if (!keep_running) return 1;

		cout << "Writing " << graph.num_nodes() << " position pages...   0%";

		foreach (n : nodenums(graph))
		{
			if (!keep_running) break;

			progress(n.index, graph.num_nodes());

			position_page::write_it(mkimg, graph, n,
				config->image_url
					? *(config->image_url)
					: "../res/");
		}

		cout << '\n';

		return !keep_running;
	}
	catch (exception const & e)
	{
		cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
