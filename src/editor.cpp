#include "editor.hpp"
#include "persistence.hpp"

namespace GrappleMap
{
	Editor::Editor(boost::program_options::variables_map const & programOptions)
		: dbFile(programOptions["db"].as<std::string>())
		, graph{loadGraph(dbFile)}
	{
		if (auto start = posinseq_by_desc(graph, programOptions["start"].as<string>()))
			location.location.segment = segment_from(*start);
		else
			throw std::runtime_error("no such position/transition");
	}

	void Editor::toggle_selected()
	{
		SeqNum const curseq = location.location.segment.sequence;

		if (selection.empty()) selection.push_back({curseq, location.reorientation});
		else if (selection.front().sequence == curseq) selection.pop_front();
		else if (selection.back().sequence == curseq) selection.pop_back();
		else if (!elem(curseq, selection))
		{
			if (graph.to(curseq).node == graph.from(selection.front().sequence).node)
			{
				selection.push_front(sequence(segment(location)));
			}
			else if (graph.from(curseq).node == graph.to(selection.back().sequence).node)
			{
				selection.push_back(sequence(segment(location)));
			}
		}

//			else if (it's after a known one) push_back();
	}

	void Editor::save()
	{
		GrappleMap::save(graph, dbFile);
		cout << "Saved.\n";
	}

	void Editor::mirror()
	{
		location.reorientation.mirror = !location.reorientation.mirror;
	}

	void Editor::delete_keyframe()
	{
		if (optional<PositionInSequence> const p = position(location.location))
		{
			push_undo();

			if (auto const new_pos = graph.erase(*p))
			{
				//todo: location.position = *new_pos;
				calcViables();
			}
			else undoStack.pop();
		}
	}

	void Editor::swap_players()
	{
		if (auto const pp = position(location.location))
		{
			push_undo();

			auto p = graph[*pp];
			GrappleMap::swap_players(p);
			graph.replace(*pp, p, true);
		}
	}

	void Editor::calcViables()
	{
		foreach (j : playerJoints)
			viables[j] = determineViables(graph, from(location.location.segment), // todo: bad
					j, nullptr, location.reorientation);
	}
	
	void Editor::insert_keyframe()
	{
		push_undo();

		graph.split_segment(location.location);
		++location.location.segment.segment;
		location.location.howFar = 0;

		calcViables();
	}

	void Editor::push_undo()
	{
		undoStack.emplace(graph, location);
	}

	void Editor::branch()
	{
		if (auto const pp = position(location.location))
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
		if (optional<PositionInSequence> const pp = position(location.location))
			graph.replace(*pp, inverse(location.reorientation)(new_pos), false);
	}

	void Editor::toggle_lock(bool const b)
	{
		lockToTransition = b;
	}

	void Editor::toggle_playback()
	{
		if (playbackLoc) playbackLoc = boost::none;
		else
		{
			if (selection.empty()) selection = {sequence(segment(location))};

			playbackLoc = from(first_segment(selection.front()));
		}
	}
}
