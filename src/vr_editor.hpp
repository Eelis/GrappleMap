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
#include "editor.hpp"
#include "rendering.hpp"

namespace GrappleMap
{
	class VrApp: public Vrui::Application
	{
		boost::program_options::variables_map programOptions;
		Editor editor;
		Style style;
		PlayerDrawer playerDrawer;

		void on_save_button(Misc::CallbackData *);
		void on_undo_button(Misc::CallbackData *);
		void on_swap_button(Misc::CallbackData *);
		void on_mirror_button(Misc::CallbackData *);
		void on_branch_button(Misc::CallbackData *);
		void on_insert_keyframe_button(Misc::CallbackData *);
		void on_delete_keyframe_button(Misc::CallbackData *);
		void on_select_button(Misc::CallbackData *);
		void on_lock_toggle(GLMotif::ToggleButton::ValueChangedCallbackData *);
		void on_playback_toggle(GLMotif::ToggleButton::ValueChangedCallbackData *);

		public:

			VrApp(int argc, char ** argv);
		
			void frame() override;
			void display(GLContextData &) const override;
			void resetNavigation() override;
			void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData *) override;
	};

	class JointEditor: public Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<V3> offset;

		public:

			JointEditor(Vrui::DraggingTool & t, Editor & e)
				: Vrui::DraggingToolAdapter{&t}, editor(e)
			{}
		
			void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *) override;
			void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
			void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};

	class BrowseTool: public Vrui::DraggingToolAdapter
	{
		Editor & editor;

		public:

			BrowseTool(Vrui::DraggingTool & t, Editor & e)
				: Vrui::DraggingToolAdapter{&t}, editor(e)
			{
				editor.calcViables();
			}
		
			void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
			void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *) override;
			void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};
}

#endif
