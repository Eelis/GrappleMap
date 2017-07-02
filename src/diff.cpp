#include "graph_util.hpp"
#include "persistence.hpp"
#include <boost/variant.hpp>

namespace GrappleMap
{
	template<typename R>
	string comma_separated(R const & r)
	{
		string s;
		bool first = true;
		foreach(x : r)
		{
			if (first) first = false; else s += ", ";
			s += x;
		}
		return s;
	}

	string name(NodeNum n, Graph const & g)
	{
		return g[n].description.empty()
			? "p" + to_string(n.index)
			: g[n].description.front();
	}

// representation of graph diffs:

	enum class PositionComparison { identical, reoriented, changed };
	enum class TransComparison { identical, adapted };
		// adapted means all non-end frames equal, and end frames mapped to same DiffNode

	struct RemovedNode { NodeNum a; };
	struct AddedNode { NodeNum b; };

	struct RemovedEdge { SeqNum a; };
	struct AddedEdge { SeqNum b; };

	struct CommonNode
	{
		NodeNum a, b;
		bool same_name, same_desc;
		PositionComparison position;
	};

	struct CommonEdge
	{
		SeqNum a, b;
		TransComparison edge;
	};

	struct DiffGraph
	{
		Graph const & ga, & gb;

		using Node = boost::variant<RemovedNode, AddedNode, CommonNode>;
		using Edge = boost::variant<RemovedEdge, AddedEdge, CommonEdge>;

		map<NodeNum, Node> nodes;
		map<SeqNum, Edge> edges;
		map<NodeNum, NodeNum> a_to_c, b_to_c;
		map<SeqNum, SeqNum> sa_to_c, sb_to_c;

		NodeNum to(SeqNum const s) const
		{
			Edge const & e = edges.at(s);

			if (auto const * a = boost::get<RemovedEdge const>(&e))
				return a_to_c.at(*ga[a->a].to);
			else if (auto const * b = boost::get<AddedEdge const>(&e))
				return b_to_c.at(*gb[b->b].to);
			else if (auto const * common = boost::get<CommonEdge const>(&e))
				return a_to_c.at(*ga[common->a].to);
			else abort();
		}

		NodeNum from(SeqNum const s) const
		{
			Edge const & e = edges.at(s);

			if (auto const * a = boost::get<RemovedEdge const>(&e))
				return a_to_c.at(*ga[a->a].from);
			else if (auto const * b = boost::get<AddedEdge const>(&e))
				return b_to_c.at(*gb[b->b].from);
			else if (auto const * common = boost::get<CommonEdge const>(&e))
				return a_to_c.at(*ga[common->a].from);
			else abort();
		}
	};

// computing diff graph:

	struct Differ
	{
		Differ(Graph const & a, Graph const & b)
			: ga(a), gb(b)
		{
			compare_positions();
			compare_transitions();
		}

		Graph const & ga, & gb;

		DiffGraph diff{ga, gb};

		optional<CommonNode> compare_position(NodeNum const a, NodeNum const b) const
		{
			CommonNode common{a, b,
				(ga[a].description.empty() && gb[b].description.empty()) ||
				(!ga[a].description.empty() && !gb[b].description.empty() &&
					ga[a].description.front() == gb[b].description.front()),
				desc(ga[a]) == desc(gb[b]) };

			if (ga[a].position == gb[b].position)
			{
				common.position = PositionComparison::identical;
				return common;
			}

			if (is_reoriented(ga[a].position, gb[b].position))
			{
				common.position = PositionComparison::reoriented;
				return common;
			}

			common.position = PositionComparison::changed;

			if (common.same_name && name(ga[a]))
				return common;

			return {};
		}

		optional<CommonEdge> compare_edge(SeqNum const a, SeqNum const b)
		{
			CommonEdge common{a, b};

			auto const & pa = ga[a].positions;
			auto const & pb = gb[b].positions;

			if (pa == pb)
			{
				common.edge = TransComparison::identical;
				return common;
			}

			vector<Position> middleA(pa.begin() + 1, pa.end() - 1);
			vector<Position> middleB(pb.begin() + 1, pb.end() - 1);

			if (middleA == middleB &&
				diff.a_to_c[*ga[a].from] == diff.b_to_c[*gb[b].from] &&
				diff.a_to_c[*ga[a].to] == diff.b_to_c[*gb[b].to])
			{
				common.edge = TransComparison::adapted;
				return common;
			}

			return {};
		}

		void compare_positions()
			// post: all A positions appear in a_to_c, analogous for B.
		{
			NodeNum c{0};

			foreach (a : nodenums(ga))
			{
				diff.a_to_c[a] = c;

				optional<CommonNode> common;

				foreach (b : nodenums(gb))
					if (common = compare_position(a, b))
					{
						diff.b_to_c[b] = c;
						break;
					}

				if (common) diff.nodes[c] = *common;
				else diff.nodes[c] = RemovedNode{a};

				++c;
			}

			foreach (b : nodenums(gb))
				if (!elem(b, diff.b_to_c))
				{
					diff.b_to_c[b] = c;
					diff.nodes[c] = AddedNode{b};
					++c;
				}
		}

		void compare_transitions()
			// post: all A sequences appear in sa_to_c, analogous for B.
		{
			SeqNum c{0};

			foreach (a : seqnums(ga))
			{
				diff.sa_to_c[a] = c;

				optional<CommonEdge> common;

				foreach (b : seqnums(gb))
					if (common = compare_edge(a, b))
					{
						diff.sb_to_c[b] = c;
						break;
					}

				if (common) diff.edges[c] = *common;
				else diff.edges[c] = RemovedEdge{a};

				++c;
			}

			foreach (b : seqnums(gb))
				if (!elem(b, diff.sb_to_c))
				{
					diff.sb_to_c[b] = c;
					diff.edges[c] = AddedEdge{b};
					++c;
				}
		}
	};

// describing diff graph as text:

	string describe(NodeNum n, Graph const & g)
	{
		string d = "p" + to_string(n.index);
		if (!g[n].description.empty())
			d += " (\"" + g[n].description[0] + "\")";
		return d;
	}

	string describe(SeqNum n, Graph const & g)
	{
		return "t" + to_string(n.index) + " (\"" + g[n].description[0] + "\")";
	}

	vector<string> list_diffs(CommonNode const & common, Graph const & ga, Graph const & gb)
	{
		vector<string> diffs;

		if (!common.same_name)
		{
			string const * nameA = name(ga[common.a]);
			string const * nameB = name(gb[common.b]);
			
			diffs.push_back("renamed from \""
				+ (nameA ? *nameA : "") + "\" to \""
				+ (nameB ? *nameB : "") + "\"");
		}

		if (!common.same_desc)
		{
			diffs.push_back("description changed"); // TODO from \"" + desc(ga[n.a]) + "\" to \"" + desc(gb[n.b]) + ", and ";
		}

		switch (common.position)
		{
			case PositionComparison::identical:
				break;
			case PositionComparison::reoriented:
				diffs.push_back("reoriented");
				break;
			case PositionComparison::changed:
				diffs.push_back("pose changed");
				break;
		}

		return diffs;
	}

	vector<string> list_diffs(CommonEdge const & common, Graph const & ga, Graph const & gb)
	{
		vector<string> diffs;

		string const & nameA = ga[common.a].description.front();
		string const & nameB = gb[common.b].description.front();

		if (nameA != nameB)
		{
			diffs.push_back("renamed from \"" + nameA + "\" to \"" + nameB + "\"");
		}

		if (common.edge == TransComparison::adapted)
		{
			diffs.push_back("adapted");
		}

		// todo
		return diffs;
	}

	string describe(DiffGraph::Node const & n, Graph const & ga, Graph const & gb)
	{
		if (auto const * a = boost::get<RemovedNode const>(&n))
			return GrappleMap::describe(a->a, ga) + " was removed";
		else if (auto const * b = boost::get<AddedNode const>(&n))
			return GrappleMap::describe(b->b, gb) + " was added"; // todo: only really interesting if it has a name
		else if (auto const * common = boost::get<CommonNode const>(&n))
			return comma_separated(list_diffs(*common, ga, gb));
		else abort();
	}

	string describe(DiffGraph::Edge const & e, Graph const & ga, Graph const & gb)
	{
		if (auto const * a = boost::get<RemovedEdge const>(&e))
			return GrappleMap::describe(a->a, ga) + " was removed";
		else if (auto const * b = boost::get<AddedEdge const>(&e))
			return GrappleMap::describe(b->b, gb) + " was added";
		else if (auto const * common = boost::get<CommonEdge const>(&e))
			return comma_separated(list_diffs(*common, ga, gb));
		else abort();
	}

	void print(DiffGraph const & diff, Graph const & ga, Graph const & gb, std::ostream & out)
	{
		out << "Nodes:\n";
		foreach (x : diff.nodes)
		{
			auto s = describe(x.second, ga, gb);
			if (!s.empty()) out << s << '\n';
		}

		out << "Edges:\n";
		foreach (x : diff.edges)
		{
			auto s = describe(x.second, ga, gb);
			if (!s.empty()) out << s << '\n';
		}
	}

// dot

	struct DotPrinter
	{
		DiffGraph const & diff;

		string color(DiffGraph::Node const & n) const
		{
			if (boost::get<RemovedNode const>(&n)) return "red";
			else if (boost::get<AddedNode const>(&n)) return "green";
			else if (auto const * common = boost::get<CommonNode const>(&n))
				return (list_diffs(*common, diff.ga, diff.gb).empty() ? "white" : "orange");
			else abort();
		}

		string color(DiffGraph::Edge const & e) const
		{
			if (boost::get<RemovedEdge const>(&e)) return "red";
			else if (boost::get<AddedEdge const>(&e)) return "green";
			else if (auto const * common = boost::get<CommonEdge const>(&e))
				return (list_diffs(*common, diff.ga, diff.gb).empty() ? "white" : "orange");
			else abort();
		}

		string label(DiffGraph::Edge const & e) const
		{
			if (auto const * a = boost::get<RemovedEdge const>(&e))
				return diff.ga[a->a].description.front();
			else if (auto const * b = boost::get<AddedEdge const>(&e))
				return diff.gb[b->b].description.front();
			else if (auto const * common = boost::get<CommonEdge const>(&e))
			{
				string l = diff.gb[common->b].description.front();
				if (diff.ga[common->a].description != diff.gb[common->b].description)
					l += "*";
				return l;
			}
			else abort();
		}

		string label(DiffGraph::Node const & e) const
		{
			if (auto const * a = boost::get<RemovedNode const>(&e))
				return name(a->a, diff.ga);
			else if (auto const * b = boost::get<AddedNode const>(&e))
				return name(b->b, diff.gb);
			else if (auto const * common = boost::get<CommonNode const>(&e))
			{
				string l = name(common->b, diff.gb);
				if (diff.ga[common->a].description != diff.gb[common->b].description)
					l += "*";
				return l;
			}
			else abort();
		}

		bool interesting(DiffGraph::Edge const & e) const
		{
			if (boost::get<RemovedEdge const>(&e)) return true;
			else if (boost::get<AddedEdge const>(&e)) return true;
			else if (auto const * common = boost::get<CommonEdge const>(&e))
				return (list_diffs(*common, diff.ga, diff.gb).empty() ? false : true);
			else abort();
		}

		bool interesting(NodeNum const n) const
		{
			foreach (e : diff.edges)
				if (interesting(e.second) &&
					(n == diff.from(e.first) || n == diff.to(e.first)))
					return true;

			return false;
		}

		void todot(std::ostream & o) const
		{
			o << "digraph G {\n";

			foreach (x : diff.nodes)
				if (interesting(x.first))
				{
					o << 'p' << x.first.index << " [";
					o << "style=filled fillcolor=" << color(x.second);
					o << ",label=\"" << label(x.second) << "\"";
					o << "];\n";
				}

			foreach (x : diff.edges)
				if (interesting(x.second))
				{
					o << diff.from(x.first) << " -> " << diff.to(x.first);
					o << "[color=" << color(x.second);
					o << ",label=\"" << label(x.second) << "\"";
					o << "];\n";
					
				}

			o << "}\n";
		}

	};

	void todot(DiffGraph const & diff, std::ostream & o)
	{
		DotPrinter{diff}.todot(o);
	}
}

int main(int const argc, char const * const * const argv)
{
	try
	{
		if (argc != 3) throw std::runtime_error("args: file file");

		auto ga = GrappleMap::loadGraph(argv[1]);
		auto gb = GrappleMap::loadGraph(argv[2]);

		todot(GrappleMap::Differ(ga, gb).diff, std::cout);
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}
