#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	namespace
	{
		Position operator*(Position p, ONTransform const & t)
		{
			using Point = Geometry::Point<double, 3>;

			foreach (j : playerJoints)
			{
				V3 & v = p[j];
				v = v3(t.transform(Point(v.x, v.y, v.z)));
			}

			return p;
		}
	}

	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		editor.push_undo();

		start_pos = editor.current_position();

		dragTransform = ONTransform(
				cb->startTransformation.getTranslation(),
				cb->startTransformation.getRotation());
		dragTransform->doInvert();

		if (joint)
			joint_edit_offset = v3(cb->startTransformation.getTranslation()) -
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

		if (!pp || !dragTransform || !start_pos) return;

		Graph const & g = editor.getGraph();
		Position new_pos = editor.current_position();
		auto const cursor = v3(cbData->currentTransformation.getTranslation());

		ONTransform transform = ONTransform(
			cbData->currentTransformation.getTranslation(),
			cbData->currentTransformation.getRotation());
		transform *= *dragTransform;

		if (joint)
		{
			if (!joint_edit_offset) return;

			PlayerJoint const j = *joint;

			auto const joint_pos = cursor - *joint_edit_offset;

			if (confined)
			{
				if (auto pr = prev(*pp))
				if (auto ne = next(*pp, g))
				{
					Position const prev_p = at(*pr, g);
					Position const next_p = at(*ne, g);
					V3 const dir = next_p[j] - prev_p[j];

					new_pos[j] = prev_p[j] + dir * std::max(0., std::min(1., closest(prev_p[j], dir, joint_pos)));
				}
			}
			else new_pos[j] = joint_pos;

			spring(new_pos, j);
		}
		else new_pos = *start_pos * transform;

		editor.replace(new_pos);
	}
}
