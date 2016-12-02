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
		string const dbFile;
		Graph graph;
		std::stack<std::pair<Graph, ReorientedLocation>> undoStack;
		Viables viables;
		Selection selection;

	public:
		ReorientedLocation location{{SegmentInSequence{{0}, 0}, 0}, {}}; // todo: hide

		optional<PlayerJoint> edit_joint;
		optional<PlayerJoint> browse_joint;
		bool lockToTransition = true;
		optional<ReorientedLocation> playbackLoc;

		explicit Editor(boost::program_options::variables_map const & programOptions);

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
		void calcViables();
		void replace(Position);

		inline Position current_position() const { return at(location, graph); }
		Graph const & getGraph() const { return graph; }
		Viables const & getViables() const { return viables; }
		ReorientedLocation const & getLocation() const { return location; }
		Selection const & getSelection() const { return selection; }
	};
}

#endif
