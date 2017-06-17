#include "editor.hpp"
#include "persistence.hpp"
#include "metadata.hpp"

namespace GrappleMap
{
	namespace
	{
		Path path_from_desc(std::string const & desc)
		{
			Path p;

			std::istringstream iss(desc);
			int32_t i;
			iss >> i; // todo: negative for reverse
			char c;
			do
				p.push_back(i >= 0
					? Reversible<SeqNum>{{SeqNum::underlying_type(i)}, false}
					: Reversible<SeqNum>{{SeqNum::underlying_type(-i)}, true});
			while (iss >> c >> i);

			return p;
		}

		unsigned segmentsPerSecond(Sequence const & s) // todo: move
		{
			return s.detailed ? 10 : 5;
		}

		double timeInSequence(Location const & l, Graph const & g)
		{
			return (l.segment.segment.index + l.howFar)
				/ segmentsPerSecond(g[l.segment.sequence]);
		}

		double duration(Sequence const & s)
		{
			return num_segments(s) / segmentsPerSecond(s);
		}

		optional<double> timeIn(OrientedPath const & p, Location const & l, Graph const & g)
		{
			double t = 0;

			foreach (s : p)
			{
				double const s_dur = duration(g[**s]);

				if (l.segment.sequence == **s)
				{
					double const tis = timeInSequence(l, g);
					return t + (s->reverse ? s_dur - tis : tis);
				}

				t += s_dur;
			}

			return boost::none;
		}
	}

	Editor::Editor(Graph g/*, string const & start_desc*/)
		: graph(std::move(g))
	{}

	void go_to_desc(string const & desc, Editor & editor)
	{
		Graph const & graph = editor.getGraph();

		/* todo

		if (desc.find(',') != string::npos)
		{
			foreach (x : path_from_desc(desc))
				selection.push_back({x, PositionReorientation{}});
		}
		else */
		
		if (auto start = named_entity(graph, desc))
		{
			/*
			if (Step const * step = boost::get<Step>(&*start))
			{
				selection = {{*step, PositionReorientation{}}};
			}
			else*/ if (NodeNum const * node = boost::get<NodeNum>(&*start))
			{
				go_to(*node, editor);
/*
				auto const & io = graph[*node].in_out;
				if (io.empty()) error("cannot edit unconnected node");
				location = {start_loc(io.front(), graph), PositionReorientation{}};
				*/
				return;

			}
			else error("unexpected type of named entity");
		}
		else error("no such position/transition");
/*
		reorient_from(selection, selection.begin(), graph);
		location = from_loc(first_segment(selection.front(), graph));
*/
	}

	void advance(Editor & e)
	{
		if (auto x = advance_along(e.getLocation(), e.getSelection(), e.getGraph()))
			e.setLocation(*x);
	}

	void retreat(Editor & e)
	{
		if (auto x = retreat_along(e.getLocation(), e.getSelection(), e.getGraph()))
			e.setLocation(*x);
	}

	bool Editor::try_extend_selection(SeqNum const sn)
	{
		Reoriented<NodeNum> n = from(selection.front(), graph);

		foreach (s : in_sequences(n, graph))
			if (**s == sn)
			{
				selection.push_front(s);
				return true;
			}

		n = to(selection.back(), graph);

		foreach (s : out_sequences(n, graph))
			if (**s == sn)
			{
				selection.push_back(s);
				return true;
			}

		return false;
	}

	void Editor::set_selected(SeqNum const sn, bool const b)
	{
		if (selection.empty())
		{
			if (b && sn == location->segment.sequence)
				selection = {nonreversed(sequence(location))};
		}
		else if (b)
		{
			if (!try_extend_selection(sn))
				return;
		}
		else if (**selection.front() == sn) selection.pop_front();
		else if (**selection.back() == sn) selection.pop_back();
		else return;

		if (playback) start_playback();
	}

	void clear_selection(Editor & e)
	{
		while (!e.getSelection().empty())
			e.set_selected(**e.getSelection().front(), false);
	}
/*
	void Editor::toggle_selected()
	{
		if (playback) return;

		Reoriented<SeqNum> const seq = sequence(location);

		Reoriented<Reversible<SeqNum>> const
			fs = nonreversed(seq),
			bs = reversed(seq);

		if (selection.empty()) selection.push_back(fs);
		else if (**selection.front() == *seq) selection.pop_front();
		else if (**selection.back() == *seq) selection.pop_back();
		else if (!elem(*seq, selection)) // because we don't want to cut in the middle
		{
			if (*to(graph, *fs) == *from(graph, *selection.front()))
			{
				selection.push_front(fs);
				return;
			}
			else if (*from(graph, *fs) == *to(graph, *selection.back()))
			{
				selection.push_back(fs);
				return;
			}
			else if (graph[*seq].bidirectional)
			{
				if (*to(graph, *bs) == *from(graph, *selection.front()))
				{
					selection.push_front(bs);
					return;
				}
				else if (*from(graph, *bs) == *to(graph, *selection.back()))
				{
					selection.push_back(bs);
					return;
				}
			}

			selection = {nonreversed(sequence(location))};
		}
	}
*/
	void Editor::mirror()
	{
		flip(location.reorientation.mirror);

		foreach (t : selection) flip(t.reorientation.mirror);
	}

	void Editor::delete_keyframe()
	{
		if (optional<PositionInSequence> const p = position(*location))
		{
			if (!selection.empty() && node_at(graph, *p) &&
				(p->position.index != 0 || **selection.front() != p->sequence) &&
				(p->position.index == 0 || **selection.back() != p->sequence))
			{
				std::cerr << "Keyframe delete action ignored because it would break selection." << std::endl;
				return;
			}

			push_undo();

			if (optional<PosNum> const new_pos = graph.erase(*p))
				go_to(PositionInSequence{p->sequence, *new_pos}, *this);
			else undoStack.pop();
		}
		else std::cerr << "Keyframe delete action ignored because not currently at keyframe." << std::endl;
	}

	void swap_players(Editor & e)
	{
		if (!position(*e.getLocation()))
		{
			std::cerr << "Player swap action ignored because not currently at keyframe." << std::endl;
			return;
		}

		e.push_undo();
		auto p = e.current_position();
		GrappleMap::swap_players(p);
		e.replace(p, Graph::NodeModifyPolicy::unintended);
	}

	void mirror_position(Editor & e)
	{
		if (!position(*e.getLocation()))
		{
			std::cerr << "Position mirror action ignored because not currently at keyframe." << std::endl;
			return;
		}

		e.push_undo();
		auto p = e.current_position();
		e.replace(mirror(p), Graph::NodeModifyPolicy::unintended);
	}

	void Editor::insert_keyframe()
	{
		push_undo();

		graph.split_segment(*location);
		++location->segment.segment;
		location->howFar = 0;

		assert(is_at_keyframe(*this));
	}

	void Editor::push_undo()
	{
		undoStack.emplace(location, selection);
		graph.rewind_point();
	}

	void Editor::branch()
	{
		if (auto const pp = position(*location))
		{
			push_undo();

			try
			{
				if (auto newseq = split_at(graph, *pp))
				{
					if (location->howFar == 0)
					{
						--location->segment.segment.index;
						location->howFar = 1;
					}

					if (auto opt_iter = currently_in_selection())
					{
						selection.insert(std::next(*opt_iter), {nonreversed(*newseq), {}});
							// todo: reverse if appropriate
						reorient_from(selection, *currently_in_selection(), graph);
					}
				}
			}
			catch (exception const & e)
			{
				cerr << "could not branch: " << e.what() << '\n';
			}
		}
		else std::cerr << "Branch action ignored because not currently at key frame. "
			"If you really want to branch here, insert a key frame here first." << std::endl;
	}

	void Editor::undo()
	{
		if (undoStack.empty()) return;

		std::tie(location, selection) = undoStack.top();
		undoStack.pop();
		graph.rewind();
	}

	optional<OrientedPath::iterator> Editor::currently_in_selection()
	{
		for (OrientedPath::iterator i = selection.begin(); i != selection.end(); ++i)
			if (***i == location->segment.sequence)
				return i;
		return {};
	}

	void Editor::replace_sequence(vector<Position> const & v)
	{
		SeqNum const seq = location->segment.sequence;

		if (optional<OrientedPath::iterator> pathi = currently_in_selection())
		{
			if (v.size() != graph[seq].positions.size())
				throw std::runtime_error("Editor::replace_sequence: wrong size");

			auto i = v.begin();

			foreach (p : positions(graph, seq))
			{
				graph.replace(p, inverse(location.reorientation)(*i), Graph::NodeModifyPolicy::propagate);
				++i;
			}

			reorient_from(selection, *pathi, graph); // todo: elsehwere, too
		}
	}

	void Editor::append_new(NodeNum const to)
	{
		if (selection.empty()) return;

		Sequence seq;
		seq.description = {"new"};
		seq.positions =
			{ graph[**selection.back()].positions.back()
			, graph[to].position };
			// todo: not for reverse steps
		seq.detailed = false;
		seq.bidirectional = false;

		SeqNum const new_seq = insert(graph, seq);
		set_selected(new_seq, true);
	}

	void Editor::prepend_new(NodeNum const src)
	{
		if (selection.empty()) return;

		Sequence seq;
		seq.description = {"new"};
		seq.positions =
			{ graph[src].position
			, graph[**selection.front()].positions.front() };
			// todo: not for reverse steps
		seq.detailed = false;
		seq.bidirectional = false;

		SeqNum const new_seq = insert(graph, seq);
		set_selected(new_seq, true);
	}

	void Editor::replace(Position const new_pos, Graph::NodeModifyPolicy const policy)
	{
		if (optional<PositionInSequence> const pp = position(*location))
		{
			if (optional<OrientedPath::iterator> i = currently_in_selection())
			{
				// we don't allow editing while outside the selection (because suppose
				// we're miles away from the selection, then figuring out the required
				// reorientations for the selection involves finding a path to there
				// from where the edit took place)

				graph.replace(*pp, inverse(location.reorientation)(new_pos), policy);
				reorient_from(selection, *i, graph);
			}
		}
	}

	void Editor::toggle_lock(bool const b)
	{
		selectionLock = b;
	}

	void Editor::start_playback()
	{
		playback.reset(new Playback(graph,
			selection.empty() ?
				OrientedPath{nonreversed(sequence(location))}
				: selection));
	}

	void Editor::toggle_playback()
	{
		if (playback) playback.reset();
		else start_playback();
	}

	void Editor::frame(double const secondsElapsed)
	{
		if (playback) playback->frame(secondsElapsed);
	}

	void Editor::set_description(NodeNum n, string const & d)
	{
		graph.set_description(n, d);
	}

	void Editor::set_description(SeqNum s, string const & d)
	{
		graph.set_description(s, d);
	}

	optional<Reoriented<Location>> Editor::playingBack() const
	{
		if (playback) return playback->location();
		return boost::none;
	}

	void Editor::setLocation(Reoriented<Location> const l)
	{
		if (playback) return;

		if (selectionLock && !elem(l->segment.sequence, selection)) return;

		location = l;
	}

	void go_to(SeqNum const seq, Editor & e)
	{
		Graph const & g = e.getGraph();

		OrientedPath const & sel = e.getSelection();

		if (!sel.empty() &&
			(*g[seq].to == *from(g, *sel.front()) ||
			 *g[seq].from == *to(g, *sel.back())))
			e.set_selected(seq, true);
			// todo: also try reverse

		foreach (s : e.getSelection())
			if (**s == seq)
			{
				e.setLocation(from_loc(first_segment(s, g)));
				return;
			}

		// for other transitions, we cannot ensure a matching
		// selection orientation, so we clear the selection
		
		e.toggle_lock(false);
		clear_selection(e);
		e.setLocation({start_loc(seq), PositionReorientation{}});
	}

	void go_to(NodeNum n, Editor & e)
	{
		Graph const & g = e.getGraph();
		if (g[n].in_out.empty()) error("cannot go to unconnected node");

		foreach (s : e.getSelection())
			if (n == *g[**s].from)
			{
				e.setLocation(from_loc(first_segment(s, g)));
				return;
			}
			else if (n == *g[**s].to)
			{
				e.setLocation(to_loc(last_segment(s, g)));
				return;
			}

		// for other nodes, we cannot ensure a matching
		// selection orientation, so we clear the selection

		e.toggle_lock(false);
		clear_selection(e);
		auto const & t = g[n].out.empty() ? g[n].in : g[n].out;
		e.setLocation({start_loc(t.front(), g), PositionReorientation{}});
	}

	void go_to(SegmentInSequence const sis, Editor & editor)
	{
		if (editor.playingBack()) return;

		foreach (s : editor.getSelection())
			if (**s == sis.sequence)
			{
				editor.setLocation(Location{sis, 0.5} * s.reorientation);
				return;
			}
	}

	void go_to(PositionInSequence const pis, Editor & editor)
	{
		if (editor.playingBack()) return;

		foreach (s : editor.getSelection())
			if (**s == pis.sequence)
			{
				editor.setLocation(pos_loc(pis, editor.getGraph()) * s.reorientation);
				return;
			}
	}

	bool snapToPos(Editor & e)
	{
		if (e.playingBack()) return false;

		Reoriented<Location> l = e.getLocation();

		double & c = l->howFar;

		double const r = std::round(c);

		if (std::abs(c - r) < 0.2)
		{
			c = r;
			e.setLocation(l);
			return true;
		}

		return false;
	}

	optional<double> timeInSelection(Editor const & e)
	{
		Location loc;

		if (optional<Reoriented<Location>> oloc = e.playingBack())
			loc = **oloc;
		else loc = *e.getLocation();

		return timeIn(e.getSelection(), loc, e.getGraph());
	}
}
