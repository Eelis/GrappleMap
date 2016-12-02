#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		if (!editor.edit_joint) return;

		editor.push_undo();

		offset = v3(cb->startTransformation.getTranslation()) -
			editor.current_position()[*editor.edit_joint];
	}

	void JointEditor::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		if (!position(editor.getLocation().location))
		{
			editor.edit_joint = boost::none;
			return;
		}

		editor.edit_joint = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);
	}

	void JointEditor::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!editor.edit_joint || !offset) return;
		
		Position new_pos = editor.current_position();
		auto cv = v3(cbData->currentTransformation.getTranslation()) - *offset;
		cv.y = std::max(0., cv.y);
		new_pos[*editor.edit_joint] = cv;
		spring(new_pos, *editor.edit_joint);

		editor.replace(new_pos);
	}
}
