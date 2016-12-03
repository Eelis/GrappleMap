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
	struct JointEditor: Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<PlayerJoint> joint;
		optional<V3> offset;

		JointEditor(Vrui::DraggingTool & t, Editor & e)
			: Vrui::DraggingToolAdapter{&t}, editor(e)
		{}
		
		void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *) override;
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};

	struct JointBrowser: Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<PlayerJoint> joint;

		JointBrowser(Vrui::DraggingTool & t, Editor & e)
			: Vrui::DraggingToolAdapter{&t}, editor(e)
		{
//			editor.calcViables();
		}
	
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};

	class VrApp: public Vrui::Application
	{
		Editor editor;
		Style style;
		PlayerDrawer playerDrawer;
		unique_ptr<JointEditor> jointEditor;
		unique_ptr<JointBrowser> jointBrowser;

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
}

#endif
