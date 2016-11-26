#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void VrApp::JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData *)
	{
		app.push_undo();
	}

	void VrApp::JointEditor::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		app.edit_joint = closest_joint(
			at(app.location, app.graph),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);
	}

	void VrApp::JointEditor::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!app.edit_joint) return;
		
		if (auto const pp = position(app.location.location))
		{
			Position new_pos = at(app.location, app.graph);
			auto cv = v3(cbData->currentTransformation.getTranslation());
			cv.y = std::max(0., cv.y);
			new_pos[*app.edit_joint] = cv;
			spring(new_pos, *app.edit_joint);
			app.graph.replace(*pp, inverse(app.location.reorientation)(new_pos), false);
		}
	}
}
