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
#include "VruiXine.hpp"

namespace GrappleMap
{
	using ONTransform = Geometry::OrthonormalTransformation<double, 3>;

	struct JointEditor: Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<PlayerJoint> joint;
		optional<V3 const> joint_edit_offset;
		optional<vector<Position> const> start_seq;
		optional<Reorientation> dragTransform;
		bool confined = false;

		JointEditor(Vrui::DraggingTool & t, Editor & e, bool confined)
			: Vrui::DraggingToolAdapter{&t}, editor(e), confined(confined)
		{}

		void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *) override;
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;

		private:

			void dragSingleJoint(V3 cursor);
			void dragAllJoints(Reorientation);
	};

	struct JointBrowser: Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<PlayerJoint> joint;
		PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> const & accessibleSegments;

		JointBrowser(Vrui::DraggingTool & t, Editor & e,
			PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> const & as)
			: Vrui::DraggingToolAdapter{&t}, editor(e), accessibleSegments(as)
		{}
	
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;
	};

	using ToggleEvent = GLMotif::ToggleButton::ValueChangedCallbackData;

	class VrApp: public Vrui::Application, public GLObject
	{
		boost::program_options::variables_map opts;
		Editor editor;
		Style style;
		PlayerDrawer playerDrawer;
		double const scale;
		unique_ptr<JointEditor> jointEditor;
		bool confineEdits = false;
		PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> accessibleSegments;
		unique_ptr<JointBrowser> jointBrowser;
		vector<Viable> viables;
		VruiXine video_player;

		void on_save_button(Misc::CallbackData *);
		void on_undo_button(Misc::CallbackData *);
		void on_swap_button(Misc::CallbackData *);
		void on_mirror_button(Misc::CallbackData *);
		void on_branch_button(Misc::CallbackData *);
		void on_insert_keyframe_button(Misc::CallbackData *);
		void on_delete_keyframe_button(Misc::CallbackData *);
		void on_select_button(Misc::CallbackData *);
		void on_lock_toggle(ToggleEvent *);
		void on_playback_toggle(ToggleEvent *);
		void on_confine_edits_toggle(ToggleEvent *);

		void initContext(GLContextData& contextData) const override
		{
			video_player.initContext(contextData, this);
		}

		void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData) override
		{
			video_player.eventCallback(eventId, cbData);
		}

		public:

			VrApp(int argc, char ** argv);
		
			void frame() override;
			void display(GLContextData &) const override;
			void resetNavigation() override;
			void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData *) override;
	};
}

#endif
