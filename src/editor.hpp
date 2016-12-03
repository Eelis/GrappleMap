#ifndef GRAPPLEMAP_EDITOR_HPP
#define GRAPPLEMAP_EDITOR_HPP

#include <stack>
#include "persistence.hpp"
#include "reoriented.hpp"
#include "viables.hpp"

namespace GrappleMap
{
	class Editor
	{
		struct Playback
		{
			Selection::const_iterator i;
			SegmentNum segment;
			double howFar;
			Position chaser;

			Reoriented<Location> location() const
			{
				return {
					Location{{***i, segment}, howFar},
					i->reorientation};
			}
		};

		string const dbFile;
		Graph graph;
		std::stack<std::pair<Graph, Reoriented<Location>>> undoStack;
		Viables viables;
		Selection selection;
		Camera const * const camera; // needed because in 2d projection it affects viables
		bool selectionLock = true;
		optional<Playback> playback;
		Reoriented<Location> location{{SegmentInSequence{{0}, 0}, 0}, {}};

		void init_playback();

	public:

		Editor(
			boost::program_options::variables_map const & programOptions,
			Camera const *);

		// read

		Graph const & getGraph() const { return graph; }
		Viables const & getViables() const { return viables; }
		Selection const & getSelection() const { return selection; }
		bool lockedToSelection() const { return selectionLock; }
		Reoriented<Location> getLocation() const;
		bool playingBack() const { return bool(playback); }

		Position current_position() const
		{
			return playback ? playback->chaser : at(location, graph);
		}

		// write

		void save();
		void toggle_selected();
		void insert_keyframe();
		void delete_keyframe();
		void undo();
		void mirror();
		void swap_players();
		void branch();
		void toggle_lock(bool);
		void toggle_playback();
		void push_undo();
		void recalcViables(); // called after camera change
		void replace(Position);
		void frame(double secondsElapsed);
		void setLocation(Reoriented<Location>);
		void snapToPos();
	};
}

#endif
