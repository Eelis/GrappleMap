#include "persistence.hpp"
#include "metadata.hpp"
#include "md5.hpp"
#include <fstream>
#include <iterator>
#include <cstring>
#include <boost/algorithm/string/trim.hpp>

namespace GrappleMap {

namespace
{
	char const base62digits[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static_assert(sizeof(base62digits) == 62 + 1, "hm");

	int fromBase62(char c)
	{
		if (c >= 'a' && c <= 'z') return c - 'a';
		if (c >= 'A' && c <= 'Z') return (c - 'A') + 26;
		if (c >= '0' && c <= '9') return (c - '0') + 52;

		error("not a base 62 digit: " + std::string(1, c));
	}

	string desc(Graph::Node const & n) // TODO: bad, tojs should not alter description strings
	{
		auto desc = n.description;
		return desc.empty() ? "?" : desc.front();
	}

	constexpr size_t encoded_pos_size = 2 * joint_count * 3 * 2 + 4 * 5;

	Position decodePosition(char const * const s)
	{
		size_t offset = 0;

		auto nextdigit = [&]
			{
				while (std::isspace(s[offset])) ++offset;
				return fromBase62(s[offset++]);
			};

		auto g = [&]
			{
				int const d0 = nextdigit() * 62;
				double d = double(d0 + nextdigit()) / 1000;
				return d;
			};

		Position p;

		foreach (j : playerJoints)
			p[j] = {g() - 2, g(), g() - 2};

		return p;
	}

	vector<Sequence> readSeqs(char const * b, char const * e)
	{
		vector<Sequence> v;

		vector<string> desc;
		bool last_was_position = false;

		unsigned line_nr = 0;

		try
		{
			while (b < e)
			{
				bool const is_position = *b == ' ';

				if (is_position)
				{
					if (!last_was_position)
					{
						assert(!desc.empty());
						auto const props = properties_in_desc(desc);

						v.push_back(Sequence
							{ move(desc)
							, vector<Position>{}
							, line_nr - desc.size()
							, props.count("detailed") != 0
							, props.count("bidirectional") != 0 });
						desc.clear();
					}

					if (size_t(e - b) < encoded_pos_size) abort();

					v.back().positions.push_back(decodePosition(b));

					b += encoded_pos_size;
					line_nr += 4;
				}
				else
				{
					char const * t = b;
					while (b != e && *b != '\n') ++b;
					
					desc.push_back(string(t, b));
					++line_nr;

					if (b != e) ++b;
				}

				last_was_position = is_position;
			}
		}
		catch (exception const & e)
		{
			error("at line " + to_string(line_nr) + ": " + e.what());
		}

		return v;
	}

	ostream & operator<<(ostream & o, Position const & p)
	{
		string s;

		auto g = [&o, &s](double const d)
			{
				int const i = int(std::round(d * 1000));
				assert(i >= 0);
				assert(i < 4000);
				s += base62digits[i / 62];
				s += base62digits[i % 62];
			};

		foreach (j : playerJoints)
		{
			g(p[j].x + 2);
			g(p[j].y);
			g(p[j].z + 2);
		}

		auto const n = s.size() / 4;

		for (int i = 0; i != 4; ++i)
			o << "    " << s.substr(i * n, n) << '\n';

		return o;
	}

	ostream & operator<<(ostream & o, Sequence const & s)
	{
		foreach (l : s.description) o << l << '\n';
		foreach (p : s.positions) o << p;
		return o;
	}
}

Graph loadGraph(char const * const b, char const * const e)
{
	vector<Sequence> edges = readSeqs(b, e);

	vector<NamedPosition> pp;

	for (auto i = edges.begin(); i != edges.end(); )
		if (i->positions.size() == 1)
		{
			pp.push_back(NamedPosition{i->positions.front(), i->description, i->line_nr});
			i = edges.erase(i);
		}
		else ++i;

	return Graph(move(pp), move(edges));
}

Graph loadGraph(std::istream & ff)
{
	std::istreambuf_iterator<char> i(ff), e;
	std::string const db(i, e);
	return loadGraph(db.data(), db.data() + db.size());
}

void writeIndex(string const filename, Graph const & g, string const & dbHash)
{
	std::ofstream f(filename);
	f << dbHash << '\n';
	foreach(n : seqnums(g))
		f << g[n].from->index << ' ' << g[n].to->index << ' ';
}

Graph loadGraph(string const filename)
{
	std::ifstream ff(filename, std::ios::binary);

	if (!ff) error(filename + ": " + std::strerror(errno));

	std::istreambuf_iterator<char> i(ff), e;
	std::string const db(i, e);

	auto const dbhash = MD5(db).hexdigest();

	string const indexFile = filename + ".index";

	vector<pair<NodeNum, NodeNum>> connections;

	if (std::ifstream f{indexFile})
	{
		std::string indexHash;
		std::getline(f, indexHash);
		if (indexHash == dbhash)
		{
			NodeNum from, to;
			while (f >> from.index >> to.index)
				connections.emplace_back(from, to);
		}
	}

	vector<Sequence> edges = readSeqs(db.data(), db.data() + db.size());

	// nodes have been read as sequences of size 1

	vector<NamedPosition> pp;

	auto const is_pos = [&](Sequence const & s){ return s.positions.size() == 1; };

	foreach (e : edges)
		if (is_pos(e))
			pp.push_back(NamedPosition{e.positions.front(), e.description, e.line_nr});
	
	edges.erase(std::remove_if(edges.begin(), edges.end(), is_pos), edges.end());

	if (!connections.empty())
		return Graph(move(pp), move(edges), connections);

	Graph g(move(pp), move(edges));
	writeIndex(indexFile, g, dbhash);
	return g;
}

void save(Graph const & g, string const filename)
{
	std::ofstream f(filename, std::ios::binary);
	save(g, f);
}

void save(Graph const & g, std::ostream & o)
{
	foreach(n : nodenums(g))
		if (!g[n].description.empty())
		{
			foreach (l : g[n].description) o << l << '\n';
			o << g[n].position;
		}

	foreach(s : seqnums(g)) o << g[s];
}

Path readScene(Graph const & graph, string const filename)
{
	std::ifstream f(filename, std::ios::binary);
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
					foreach (step : graph[*prev_node].out)
						if (*to(step, graph) == *n)
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
				prev_node = *to(*step, graph);
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

		o	<< 'p' << n.index << " [";

		if (highlight) o << "style=filled fillcolor=lightgreen";

		o	<< " label=<<TABLE BORDER=\"0\"><TR>"
			<< "<TD TARGET=\"_top\" HREF=\"p" << n.index <<  heading << ".html\">";

		if (!graph[n].description.empty())
			o << replace_all(replace_all(graph[n].description.front(), "\\n", "<BR/>"), "&", "&amp;");
		else
			o << n.index;

		o << "</TD></TR></TABLE>>];\n";
	}

	foreach(s : seqnums(graph))
	{
		auto & e = graph[s];

		if (!nodes.count(*e.from) || !nodes.count(*e.to)) continue;

		auto const d = e.description.front();

		o << *e.from << " -> " << *e.to
		  << " [label=\"" << (d == "..." ? "" : d) << "\"";

		if (is_top_move(e)) o << ",color=red";
		else if (is_bottom_move(e)) o << ",color=blue";

		if (e.bidirectional) o << ",dir=\"both\"";

		o << "];\n";
	}

	o << "}\n";
}

}
