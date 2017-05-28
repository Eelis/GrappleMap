#ifndef GRAPPLEMAP_EDITOR_HPP
#define GRAPPLEMAP_EDITOR_HPP

#include <stack>
#include "persistence.hpp"
#include "reoriented.hpp"
#include "playback.hpp"

namespace GrappleMap
{
	class Editor
	{
		string const dbFile;
		Graph graph;
		std::stack<std::tuple<Graph, Reoriented<Location>, OrientedPath>> undoStack;
		OrientedPath selection;
		bool selectionLock = true;
		unique_ptr<Playback> playback;
		Reoriented<Location> location{{SegmentInSequence{{0}, 0}, 0}, {}};

		optional<OrientedPath::iterator> currently_in_selection();

	public:

		explicit Editor(string const & dbFile, string const & startDesc);

		// read

		Graph const & getGraph() const { return graph; }
		OrientedPath const & getSelection() const { return selection; }
		bool lockedToSelection() const { return selectionLock; }
		Reoriented<Location> const & getLocation() const { return location; }
		optional<Reoriented<Location>> playingBack() const;

		Position current_position() const
		{
			return playback ? playback->getPosition() : at(location, graph);
		}

		// write

		void save();
		void toggle_selected();
		void insert_keyframe();
		void delete_keyframe();
		void undo();
		void mirror(); // changes editor perspective, does not change current position
		void branch();
		void toggle_lock(bool);
		void toggle_playback();
		void push_undo();
		void replace(Position);
		void replace_sequence(vector<Position> const &);
		void frame(double secondsElapsed);
		void setLocation(Reoriented<Location>);
	};

	bool snapToPos(Editor &);
	void retreat(Editor &);
	void advance(Editor &);
	optional<double> timeInSelection(Editor const &);

	void swap_players(Editor &);
	void mirror_position(Editor &);

	inline bool is_at_keyframe(Editor const & e)
	{
		return bool(position(*e.getLocation()));
	}
}

#endif
