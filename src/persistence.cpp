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

		throw std::runtime_error("not a base 62 digit: " + std::string(1, c));
	}

	Position decodePosition(string s)
	{
		if (s.size() != 2 * joint_count * 3 * 2)
			error("position string has incorrect size " + to_string(s.size()));

		size_t offset = 0;

		auto g = [&]
			{
				double d = double(fromBase62(s[offset]) * 62 + fromBase62(s[offset + 1])) / 1000;
				offset += 2;
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

		try
		{
			while (std::getline(i, line))
			{
				++line_nr;
				bool const is_position = line.front() == ' ';

				if (is_position)
				{
					if (!last_was_position)
					{
						assert(!desc.empty());
						v.push_back(Sequence
							{ desc
							, vector<Position>{}
							, line_nr - desc.size()
							, properties_in_desc(desc).count("detailed") != 0
							, properties_in_desc(desc).count("bidirectional") != 0 });
						desc.clear();
					}

					boost::algorithm::trim(line);

					for (int j = 0; j != 3; ++j)
					{
						++line_nr;
						string more;
						if (!std::getline(i, more))
							error("could not read position at line " + to_string(line_nr));
						boost::algorithm::trim(more);
						line += more;
					}

					if (v.empty()) error("malformed file");

					v.back().positions.push_back(decodePosition(line));
				}
				else desc.push_back(line);

				last_was_position = is_position;
			}
		}
		catch (exception const & e)
		{
			error("at line " + to_string(line_nr) + ": " + e.what());
		}

		return i;
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

Graph loadGraph(std::istream & ff)
{
	vector<Sequence> edges;

	ff >> edges;

	// nodes have been read as sequences of size 1

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

	auto const dbhash = MD5(db).hexdigest();//to_string(std::hash<string>()(db));

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

	vector<Sequence> edges;

	{ std::istringstream iss(db); iss >> edges; }

	// nodes have been read as sequences of size 1

	vector<NamedPosition> pp;

	for (auto i = edges.begin(); i != edges.end(); )
		if (i->positions.size() == 1)
		{
			pp.push_back(NamedPosition{i->positions.front(), i->description, i->line_nr});
			i = edges.erase(i);
		}
		else ++i;

	if (!connections.empty())
	{
		Graph g(move(pp), move(edges), connections);
		return g;
	}

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
