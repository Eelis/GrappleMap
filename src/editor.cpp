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

	Editor::Editor(string const & dbFile, string const & start_desc)
		: dbFile(dbFile)
		, graph{loadGraph(dbFile)}
	{
		if (start_desc.find(',') != string::npos)
		{
			foreach (x : path_from_desc(start_desc))
				selection.push_back({x, PositionReorientation{}});
		}
		else if (auto start = named_entity(graph, start_desc))
		{
			if (Step const * step = boost::get<Step>(&*start))
			{
				selection = {{*step, PositionReorientation{}}};
			}
			else if (NodeNum const * node = boost::get<NodeNum>(&*start))
			{
				auto const & io = graph[*node].in_out;
				if (io.empty()) error("cannot edit unconnected node");
				location = {start_loc(io.front(), graph), PositionReorientation{}};
				return;

			}
			else error("unexpected type of named entity");
		}
		else error("no such position/transition");

		reorient_from(selection, selection.begin(), graph);
		location = from_loc(first_segment(selection.front(), graph));
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

	void Editor::set_selected(SeqNum const sn, bool const b)
	{
		if (playback) return;

		if (b)
		{
			Reoriented<NodeNum> n = from(selection.front(), graph);

			foreach (s : in_sequences(n, graph))
				if (**s == sn)
				{
					selection.push_front(s);
					return;
				}

			n = to(selection.back(), graph);

			foreach (s : out_sequences(n, graph))
				if (**s == sn)
				{
					selection.push_back(s);
					return;
				}
		}
		else
		{
			if (**selection.front() == sn) selection.pop_front();
			else if (**selection.back() == sn) selection.pop_back();
		}
	}

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

	void Editor::save()
	{
		GrappleMap::save(graph, dbFile);
		cout << "Saved.\n";
	}

	void Editor::mirror()
	{
		flip(location.reorientation.mirror);

		foreach (t : selection) flip(t.reorientation.mirror);
	}

	void Editor::delete_keyframe()
	{
		if (optional<PositionInSequence> const p = position(*location))
		{
			push_undo();

			if (auto const new_pos = graph.erase(*p))
			{
				//todo: location.position = *new_pos;
			}
			else undoStack.pop();
		}
	}

	void swap_players(Editor & e)
	{
		if (!position(*e.getLocation())) return;

		e.push_undo();
		auto p = e.current_position();
		GrappleMap::swap_players(p);
		e.replace(p);
	}

	void mirror_position(Editor & e)
	{
		if (!position(*e.getLocation())) return;

		e.push_undo();
		auto p = e.current_position();
		e.replace(mirror(p));
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
		#ifndef EMSCRIPTEN
			undoStack.emplace(graph, location, selection);
			// todo: eats too much memory for emscripten, need a leaner approach
		#endif
	}

	void Editor::branch()
	{
		if (auto const pp = position(*location))
		{
			push_undo();

			try
			{
				split_at(graph, *pp);
			}
			catch (exception const & e)
			{
				cerr << "could not branch: " << e.what() << '\n';
			}
		}
	}

	void Editor::undo()
	{
		if (undoStack.empty()) return;

		std::tie(graph, location, selection) = undoStack.top();
		undoStack.pop();
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
				graph.replace(p, inverse(location.reorientation)(*i), false);
				++i;
			}

			reorient_from(selection, *pathi, graph); // todo: elsehwere, too
		}
	}

	void Editor::replace(Position const new_pos)
	{
		if (optional<PositionInSequence> const pp = position(*location))
		{
			if (optional<OrientedPath::iterator> i = currently_in_selection())
			{
				// we don't allow editing while outside the selection (because suppose
				// we're miles away from the selection, then figuring out the required
				// reorientations for the selection involves finding a path to there
				// from where the edit took place)

				graph.replace(*pp, inverse(location.reorientation)(new_pos), false);
				reorient_from(selection, *i, graph);
			}
		}
	}

	void Editor::toggle_lock(bool const b)
	{
		selectionLock = b;
	}

	void Editor::toggle_playback()
	{
		if (playback) playback.reset();
		else
		{
			if (selection.empty())
				selection = {nonreversed(sequence(location))};

			playback.reset(new Playback(graph, selection));
		}
	}

	void Editor::frame(double const secondsElapsed)
	{
		if (playback) playback->frame(secondsElapsed);
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

	void Editor::go_to(PositionInSequence const pis)
	{
		if (playback) return;

		foreach (s : selection)
			if (**s == pis.sequence)
			{
				SegmentInSequence const sis{pis.sequence, SegmentNum{pis.position.index}};
				setLocation(Location{sis, 0} * s.reorientation);
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
