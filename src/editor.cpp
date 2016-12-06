#include "editor.hpp"
#include "persistence.hpp"

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
	}

	Editor::Editor(boost::program_options::variables_map const & programOptions, Camera const * cam)
		: dbFile(programOptions["db"].as<string>())
		, graph{loadGraph(dbFile)}
		, camera(cam)
	{
		string const start_desc = programOptions["start"].as<string>();

		if (start_desc.find(',') != string::npos)
		{
			selection = orient(path_from_desc(start_desc), graph);
			location = from_loc(first_segment(selection.front(), graph));
		}
		else if (auto start = posinseq_by_desc(graph, programOptions["start"].as<string>()))
			location->segment = segment_from(*start);
		else
			throw std::runtime_error("no such position/transition");
	}

	void Editor::toggle_selected()
	{
		if (playback) return;

		Reoriented<SeqNum> const
			seq = sequence(segment(location));

		Reoriented<Step> const
			fs = forwardStep(seq),
			bs = backStep(seq);

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
			else if (is_bidirectional(graph[*seq]))
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

			selection = {forwardStep(sequence(segment(location)))};
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

		recalcViables();
	}

	void Editor::delete_keyframe()
	{
		if (optional<PositionInSequence> const p = position(*location))
		{
			push_undo();

			if (auto const new_pos = graph.erase(*p))
			{
				//todo: location.position = *new_pos;
				recalcViables();
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

	void Editor::recalcViables()
	{
		foreach (j : playerJoints)
			viables[j] = determineViables(graph, from_pos(segment(location)), // todo: bad
					j, camera);
	}
	
	void Editor::insert_keyframe()
	{
		push_undo();

		graph.split_segment(*location);
		++location->segment.segment;
		location->howFar = 0;

		recalcViables();
	}

	void Editor::push_undo()
	{
		undoStack.emplace(graph, location);
	}

	void Editor::branch()
	{
		if (auto const pp = position(*location))
		{
			push_undo();

			try { split_at(graph, *pp); }
			catch (exception const & e)
			{
				cerr << "could not branch: " << e.what() << '\n';
			}
		}
	}

	void Editor::undo()
	{
		if (undoStack.empty()) return;

		std::tie(graph, location) = undoStack.top();
		undoStack.pop();
	}

	void Editor::replace(Position const new_pos)
	{
		if (optional<PositionInSequence> const pp = position(*location))
			graph.replace(*pp, inverse(location.reorientation)(new_pos), false);
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
				selection = {forwardStep(sequence(segment(location)))};

			playback.reset(new Playback(graph, selection));
		}
	}

	void Editor::frame(double const secondsElapsed)
	{
		if (playback) playback->frame(secondsElapsed);
	}

	Reoriented<Location> Editor::getLocation() const
	{
		return location;
	}

	void Editor::setLocation(Reoriented<Location> const l)
	{
		if (playback) return;

		if (selectionLock && !elem(l->segment.sequence, selection)) return;

		bool const differentSegment = (l->segment != location->segment);

		location = l;

		if (differentSegment) recalcViables();
	}

	void Editor::snapToPos()
	{
		if (playback) return;

		double & c = location->howFar;

		double const r = std::round(c);

		if (std::abs(c - r) < 0.2) c = r;
	}
}
