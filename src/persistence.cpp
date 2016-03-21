#include "persistence.hpp"
#include "graph_util.hpp"
#include <fstream>
#include <iterator>
#include <cstring>

namespace
{
	char const base62digits[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static_assert(sizeof(base62digits) == 62 + 1, "hm");

	int fromBase62(char c)
	{
		if (c >= 'a' && c <= 'z') return c - 'a';
		if (c >= 'A' && c <= 'Z') return (c - 'A') + 26;
		if (c >= '0' && c <= '9') return (c - '0') + 52;
		
		throw std::runtime_error("not a base 62 digit: " + std::string(1, c));
	}

	string desc(Graph::Node const & n) // TODO: bad, tojs should not alter description strings
	{
		auto desc = n.description;
		return desc.empty() ? "?" : desc.front();
	}
}

Position decodePosition(string s)
{
	if (s.size() != 2 * joint_count * 3 * 2)
		throw std::runtime_error("position string has incorrect size");

	auto g = [&s]
		{
			double d = double(fromBase62(s[0]) * 62 + fromBase62(s[1])) / 1000;
			s.erase(0, 2);
			return d;
		};

	Position p;

	foreach (j : playerJoints)
		p[j] = {g() - 2, g(), g() - 2};

	return p;
}

istream & operator>>(istream & i, vector<Sequence> & v)
{
	string line;
	vector<string> desc;
	bool last_was_position = false;

	unsigned line_nr = 0;

	while(std::getline(i, line))
	{
		++line_nr;
		bool const is_position = line.front() == ' ';

		if (is_position)
		{
			if (!last_was_position)
			{
				assert(!desc.empty());
				v.push_back(Sequence{desc, {}, line_nr - desc.size()});
				desc.clear();
			}

			while (line.front() == ' ') line.erase(0, 1);

			if (v.empty()) error("malformed file");

			v.back().positions.push_back(decodePosition(line));
		}
		else desc.push_back(line);

		last_was_position = is_position;
	}

	return i;
}

ostream & operator<<(ostream & o, Position const & p)
{
	o << "    ";

	auto g = [&o](double const d)
		{
			int const i = int(std::round(d * 1000));
			assert(i >= 0);
			assert(i < 4000);
			o << base62digits[i / 62] << base62digits[i % 62];
		};

	foreach (j : playerJoints)
	{
		g(p[j].x + 2);
		g(p[j].y);
		g(p[j].z + 2);
	}

	return o << '\n';
}

ostream & operator<<(ostream & o, Sequence const & s)
{
	foreach (l : s.description) o << l << '\n';
	foreach (p : s.positions) o << p;
	return o;
}

Graph loadGraph(string const filename)
{
	std::vector<Sequence> edges;
	std::ifstream ff(filename);

	if (!ff) error(filename + ": " + std::strerror(errno));

	ff >> edges;

	// nodes have been read as sequences of size 1

	std::vector<Graph::Node> nodes;

	for (auto i = edges.begin(); i != edges.end(); )
	{
		if (i->positions.size() == 1)
		{
			nodes.push_back(Graph::Node{i->positions.front(), i->description});
			i = edges.erase(i);
		}
		else ++i;
	}

	return Graph(nodes, edges);
}

void save(Graph const & g, string const filename)
{
	std::ofstream f(filename);

	foreach(n : nodenums(g))
		if (!g[n].description.empty())
		{
			foreach (l : g[n].description) f << l << '\n';
			f << g[n].position;
		}

	foreach(s : seqnums(g)) f << g[s];
}

Path readScene(Graph const & graph, string const filename)
{
	std::ifstream f(filename);
	if (!f) error(filename + ": " + std::strerror(errno));

	optional<NodeNum> prev_node;

	Path path;
	string desc;
	unsigned lineNr = 0;

	while (++lineNr, std::getline(f, desc))
		try
		{
			if (optional<NodeNum> const n = node_by_desc(graph, desc))
			{
				if (prev_node)
				{
					foreach (step : out_steps(graph, *prev_node))
						if (to(graph, step).node == *n)
						{
							path.push_back(step);
							goto found;
						}

					error("could not find transition");

					found:;
				}

				prev_node = n;
			}
			else if (auto const step = step_by_desc(graph, desc, prev_node))
			{
				path.push_back(*step);
				prev_node = to(graph, *step).node;
			}
			else error("unknown: \"" + desc + '"');
		}
		catch (std::exception const & e)
		{
			error(filename + ": line " + to_string(lineNr) + ": " + e.what());
		}

	return path;
}

void todot(Graph const & graph, std::ostream & o, std::map<NodeNum, bool /* highlight */> const & nodes, char const heading)
{
	o << "digraph G {rankdir=\"LR\";\n";

	foreach (p : nodes)
	{
		NodeNum const n = p.first;
		bool const highlight = p.second;

		o	<< n.index << " [";

		if (highlight) o << "style=filled fillcolor=lightgrey";

		o	<< " label=<<TABLE BORDER=\"0\"><TR>"
			<< "<TD HREF=\"p" << n.index <<  heading << ".html\">";

		if (!graph[n].description.empty())
			o << replace_all(replace_all(graph[n].description.front(), "\\n", "<BR/>"), "&", "&amp;");
		else
			o << n.index;

		o << "</TD></TR></TABLE>>];\n";
	}

	foreach(s : seqnums(graph))
	{
		auto const
			from = graph.from(s).node,
			to = graph.to(s).node;

		if (!nodes.count(from) || !nodes.count(to)) continue;

		auto const d = graph[s].description.front();

		o << from.index << " -> " << to.index
		  << " [label=\"" << (d == "..." ? "" : d) << "\"";

		if (is_top_move(graph[s]))
			o << ",color=red";
		else if (is_bottom_move(graph[s]))
			o << ",color=blue";

		if (is_bidirectional(graph[s]))
			o << ",dir=\"both\"";

		o << "];\n";
	}

	o << "}\n";
}

void tojs(V3 const v, std::ostream & js)
{
	js << "v3(" << v.x << "," << v.y << "," << v.z << ")";
}

void tojs(PositionReorientation const & reo, std::ostream & js)
{
	js
		<< "{mirror:" << reo.mirror
		<< ",swap_players:" << reo.swap_players
		<< ",angle:" << reo.reorientation.angle
		<< ",offset:";
		
	tojs(reo.reorientation.offset, js);

	js << "}\n";
}

void tojs(Position const & p, std::ostream & js)
{
	js << '[';

	for (int player = 0; player != 2; ++player)
	{
		js << '[';

		foreach (j : joints)
		{
			tojs(p[player][j], js);
			js << ',';
		}

		js << "],";
	}

	js << ']';
}

void tojs(Step const s, std::ostream & js)
{
	js << "{transition:" << s.seq.index << ",reverse:" << s.reverse << '}';
}

void tojs(ReorientedNode const & n, std::ostream & js)
{
	js << "{node:" << n.node.index;
	js << ",reo:";
	tojs(n.reorientation, js);
	js << '}';
}

void tojs(Graph const & graph, std::ostream & js)
{
	js << "nodes=[";
	foreach (n : nodenums(graph))
	{
		js << "{incoming:[";
		foreach (s : in_steps(graph, n)) { tojs(s, js); js << ','; }
		js << "],outgoing:[";
		foreach (s : out_steps(graph, n)) { tojs(s, js); js << ','; }
		js << "],position:";
		tojs(graph[n].position, js);
		js << ",description:'" << replace_all(desc(graph[n]), "'", "&#39;") << "'";
		js << "},\n";
	}
	js << "];\n\n";

	js << "transitions=[";
	foreach (s : seqnums(graph))
	{
		js << "{from:";
		tojs(graph.from(s), js);
		js << ",to:";
		tojs(graph.to(s), js);
		js << ",frames:[";
		foreach (pos : graph[s].positions)
		{
			tojs(pos, js);
			js << ',';
		}
		js << "],description:'" << replace_all(graph[s].description.front(), "'", "\\'") << '\'';
		js << "},\n";
	}
	js << "];\n\n";
}
