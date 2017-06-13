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
		Graph graph;
		std::stack<std::tuple<Reoriented<Location>, OrientedPath>> undoStack;
		OrientedPath selection;
		bool selectionLock = false;
		unique_ptr<Playback> playback;
		Reoriented<Location> location{{SegmentInSequence{{0}, 0}, 0}, {}};

		optional<OrientedPath::iterator> currently_in_selection();
		void start_playback();
		bool try_extend_selection(SeqNum);

	public:

		explicit Editor(Graph);

		Editor(Editor &&) = default;
		Editor & operator=(Editor &&) = default;

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

		void set_selected(SeqNum, bool);
			/* no-op if the specified sequence is not part of or
			   connected to the current selection or current sequence */
		void insert_keyframe();
		void delete_keyframe();
		void undo();
		void mirror(); // changes editor perspective, does not change current position
		void branch();
		void toggle_lock(bool);
		void toggle_playback();
		void push_undo();
		void append_new(NodeNum to);
		void prepend_new(NodeNum from);
		void replace(Position, Graph::NodeModifyPolicy);
		void replace_sequence(vector<Position> const &);
		void frame(double secondsElapsed);
		void setLocation(Reoriented<Location>);
	};

	void go_to(SeqNum, Editor &);
	void go_to(NodeNum, Editor &);
	void go_to(PositionInSequence, Editor &);
	void go_to(SegmentInSequence, Editor &);
	void go_to_desc(string const & desc, Editor &);

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

	inline void set_playing(Editor & e, bool b)
	{
		if (bool(e.playingBack()) != b) e.toggle_playback();
	}
}

#endif
