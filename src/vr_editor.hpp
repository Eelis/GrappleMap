#ifndef GRAPPLEMAP_VR_EDITOR_HPP
#define GRAPPLEMAP_VR_EDITOR_HPP

#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/DraggingToolAdapter.h>
#include <GLMotif/ToggleButton.h>
#include <Misc/CallbackList.h>
#include <stack>
#include "persistence.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"

namespace GrappleMap
{
	class VrApp: public Vrui::Application
	{
		boost::program_options::variables_map programOptions;
		string const dbFile;
		Graph graph;
		Style style;
		optional<PlayerJoint> edit_joint;
		ReorientedLocation location{{SegmentInSequence{{0}, 0}, 0}, {}};
		Viables viables;
		optional<PlayerJoint> browse_joint;
		bool browseMode = false;
		std::stack<std::pair<Graph, ReorientedLocation>> undo;

		class JointEditor;
		class BrowseTool;

		void on_save_button(Misc::CallbackData *);
		void on_undo_button(Misc::CallbackData *);
		void on_swap_button(Misc::CallbackData *);
		void on_mirror_button(Misc::CallbackData *);
		void on_branch_button(Misc::CallbackData *);
		void on_insert_keyframe_button(Misc::CallbackData *);
		void on_browse_toggle(GLMotif::ToggleButton::ValueChangedCallbackData *);

		void push_undo() { undo.emplace(graph, location); }

		public:

			VrApp(int argc, char ** argv);
		
			void display(GLContextData &) const override;
			void resetNavigation() override;
			void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData *) override;
	};

	class VrApp::JointEditor: public Vrui::DraggingToolAdapter
	{
		VrApp & app;

		optional<V3> offset;

		public:

			JointEditor(Vrui::DraggingTool & t, VrApp & a)
				: Vrui::DraggingToolAdapter{&t}, app(a)
			{}
		
			void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *) override;
			void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
			void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};

	class VrApp::BrowseTool: public Vrui::DraggingToolAdapter
	{
		VrApp & app;

		void calcViables();

		public:

			BrowseTool(Vrui::DraggingTool & t, VrApp & a)
				: Vrui::DraggingToolAdapter{&t}, app(a)
			{
				calcViables();
			}
		
			void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
			void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *) override;
			void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};
}

#endif
