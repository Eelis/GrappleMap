#include "persistence.hpp"
#include "util.hpp"
#include "graph.hpp"
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

	while(std::getline(i, line))
	{
		bool const is_position = line.front() == ' ';

		if (is_position)
		{
			if (!last_was_position)
			{
				assert(!desc.empty());
				v.push_back(Sequence{desc, {}});
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

	for (NodeNum n{0}; n.index != g.num_nodes(); ++n.index)
		if (!g[n].description.empty())
		{
			foreach (l : g[n].description) f << l << '\n';
			f << g[n].position;
		}

	for (SeqNum s{0}; s.index != g.num_sequences(); ++s.index) f << g[s];
}

vector<SeqNum> readScript(Graph const & graph, string const filename)
{
	std::ifstream f(filename);
	if (!f) error(filename + ": " + std::strerror(errno));

	vector<SeqNum> r;
	string seq;
	unsigned lineNr = 0;

	while (++lineNr, std::getline(f, seq))
		if (std::all_of(seq.begin(), seq.end(), (int(*)(int))std::isdigit))
			r.push_back(SeqNum{unsigned(std::stoul(seq))});
		else if (optional<SeqNum> const sn = seq_by_desc(graph, seq))
			r.push_back(*sn);
		else error(
			filename + ": line " + to_string(lineNr)
			+ ": unknown sequence: \"" + seq + '"');

	return r;
}

void todot(Graph const & graph, std::ostream & o, optional<pair<NodeNum, unsigned /* depth */>> const focus)
{
	set<NodeNum> visited;

	if (focus)
		visited = nodes_around(graph, focus->first, focus->second);
	else
		for (NodeNum n{0}; n.index != graph.num_nodes(); ++n.index)
			visited.insert(n);

	o << "digraph G {\n";

	foreach (n : visited)
		if (!graph[n].description.empty())
		
			o	<< n.index << " [label=<<TABLE BORDER=\"0\"><TR>"
				<< "<TD HREF=\"p" << n.index <<  "n.html\">"
				<< replace_all(graph[n].description.front(), "\\n", "<BR/>")
				<< "</TD></TR></TABLE>>];\n";
//			o << n.index << " [label=\"" << graph[n].description.front() /*<< " (" << n.index << ")"*/ << "\"];\n";

	for (SeqNum s{0}; s.index != graph.num_sequences(); ++s.index)
	{
		auto const
			from = graph.from(s).node,
			to = graph.to(s).node;

		if (!visited.count(from) || !visited.count(to)) continue;

		o << from.index << " -> " << to.index
		  << " [label=\"" << graph[s].description.front() /*<< " (" << s.index << ", " << graph[s].positions.size()-2 << ")"*/ << "\"];\n";
	}

	o << "}\n";
}
