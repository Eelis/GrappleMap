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
		pair<PlayerJoint, double> closest_joint;
		optional<V3 const> joint_edit_offset;
		optional<vector<Position> const> start_seq;
		optional<Reorientation> dragTransform;
		bool confined = false;
		bool keyframeInsertionEnabled = false;

		JointEditor(Vrui::DraggingTool & t, Editor & e)
			: Vrui::DraggingToolAdapter{&t}, editor(e)
		{}

		void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *) override;
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;

		bool draggingSingleJoint() const
		{
			return closest_joint.second < 0.1 * 0.1;
		}
		
		bool draggingAllJoints() const
		{
			return closest_joint.second > 0.5 * 0.5;
		}

		private:

			void dragSingleJoint(V3 cursor);
			void dragAllJoints(Reorientation);
	};

	using AccessibleSegments = PerPlayerJoint<vector<Reoriented<SegmentInSequence>>>;

	struct JointBrowser: Vrui::DraggingToolAdapter
	{
		Editor & editor;
		optional<PlayerJoint> joint;
		AccessibleSegments const & accessibleSegments;
		VruiXine * video_player;

		JointBrowser(Vrui::DraggingTool & t, Editor & e,
			AccessibleSegments const & as, VruiXine * vp)
			: Vrui::DraggingToolAdapter{&t}, editor(e)
			, accessibleSegments(as), video_player(vp)
		{}
	
		void dragCallback(Vrui::DraggingTool::DragCallbackData *) override;
		void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *) override;
		void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData *) override;

		private:

			void seek() const;
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
		bool keyframeInsertionEnabled = false;
		AccessibleSegments accessibleSegments;
		unique_ptr<JointBrowser> jointBrowser;
		vector<Viable> viables;
		unique_ptr<VruiXine> video_player;
		GLMotif::PopupWindow * editorControlDialog;

		void on_save_button(Misc::CallbackData *);
		void on_undo_button(Misc::CallbackData *);
		void on_swap_button(Misc::CallbackData *);
		void on_mirror_button(Misc::CallbackData *);
		void on_interpolate_button(Misc::CallbackData *);
		void on_branch_button(Misc::CallbackData *);
		void on_delete_keyframe_button(Misc::CallbackData *);
		void on_select_button(Misc::CallbackData *);
		void on_lock_toggle(ToggleEvent *);
		void on_playback_toggle(ToggleEvent *);
		void on_confine_edits_toggle(ToggleEvent *);
		void on_keyframe_insertion_enabled_toggle(ToggleEvent *);
		void on_sync_video_toggle(ToggleEvent *);

		GLMotif::PopupWindow * createEditorControlDialog();

		void initContext(GLContextData& contextData) const override
		{
			if (video_player)
				video_player->initContext(contextData, this);
		}

		void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData) override
		{
			if (video_player)
				video_player->eventCallback(eventId, cbData);
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
