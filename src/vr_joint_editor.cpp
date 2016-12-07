#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		if (!joint) return;

		editor.push_undo();

		offset = v3(cb->startTransformation.getTranslation()) -
			editor.current_position()[*joint];
	}

	void JointEditor::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		if (!position(*editor.getLocation()))
		{
			joint = boost::none;
			return;
		}

		joint = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);
	}

	void JointEditor::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		optional<Reoriented<PositionInSequence>> const
			pp = position(editor.getLocation());

		if (!joint || !offset || !pp) return;

		PlayerJoint const j = *joint;

		Graph const & g = editor.getGraph();

		Position new_pos = editor.current_position();
		auto cursor = v3(cbData->currentTransformation.getTranslation()) - *offset;

		if (confined)
		{
			if (auto pr = prev(*pp))
			if (auto ne = next(*pp, g))
			{
				Position const prev_p = at(*pr, g);
				Position const next_p = at(*ne, g);
				V3 const dir = next_p[j] - prev_p[j];

				new_pos[j] = prev_p[j] + dir * std::max(0., std::min(1., closest(prev_p[j], dir, cursor)));
			}
		}
		else new_pos[j] = cursor;

		spring(new_pos, j);

		editor.replace(new_pos);
	}
}
