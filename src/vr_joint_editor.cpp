#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		if (!joint) return;

		editor.push_undo();

		offset = v3(cb->startTransformation.getTranslation()) -
			current_position(editor)[*joint];
	}

	void JointEditor::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		if (!position(*editor.getLocation()))
		{
			joint = boost::none;
			return;
		}

		joint = closest_joint(
			current_position(editor),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);
	}

	void JointEditor::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!joint || !offset) return;
		
		Position new_pos = current_position(editor);
		auto cv = v3(cbData->currentTransformation.getTranslation()) - *offset;
		cv.y = std::max(0., cv.y);
		new_pos[*joint] = cv;
		spring(new_pos, *joint);

		editor.replace(new_pos);
	}
}
