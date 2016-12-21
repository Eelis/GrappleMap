#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	namespace
	{
		using VruiRotation = Geometry::Rotation<double, 3>;
		using VruiVector = Geometry::Vector<double, 3>;

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

		double y_only(VruiRotation const & r)
		{
			Geometry::Vector<double, 3> v(1,0,0);
			auto w = r.transform(v);
			return std::atan2(w[0], w[2]);
		}

		V3 xz_only(VruiVector const v)
		{
			return V3{v[0], 0, v[2]};
		}

		Reorientation reorientationTransformation(
			Geometry::OrthogonalTransformation<double, 3> const & t)
		{
			return Reorientation(xz_only(t.getTranslation()), y_only(t.getRotation()));
		}
	}

	void JointEditor::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		closest_joint = GrappleMap::closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()));
	}

	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		editor.push_undo();

		Graph const & g = editor.getGraph();

		if (!is_at_keyframe(editor))
		{
			if (!keyframeInsertionEnabled) return;

			editor.insert_keyframe();
			// todo: app.video_sync();

			Position p = editor.current_position();
			for(int i = 0; i != 30; ++i) spring(p);
			editor.replace(p);

			closest_joint = GrappleMap::closest_joint(
				editor.current_position(),
				v3(cb->startTransformation.getTranslation()));
		}

		{
			vector<Position> v;
			foreach (p : positions(g, sequence(segment(editor.getLocation()))))
				v.push_back(at(p, g));
			start_seq.emplace(v);
		}

		dragTransform = inverse(reorientationTransformation(cb->startTransformation));

		joint_edit_offset.emplace(v3(cb->startTransformation.getTranslation()) -
			editor.current_position()[closest_joint.first]);
	}

	void JointEditor::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!position(*editor.getLocation())) return;

		if (draggingSingleJoint())
			dragSingleJoint(v3(cbData->currentTransformation.getTranslation()));
		else if (draggingAllJoints() && dragTransform)
			dragAllJoints(compose(*dragTransform,
				reorientationTransformation(cbData->currentTransformation)));
	}

	void JointEditor::dragSingleJoint(V3 const cursor)
	{
		if (!joint_edit_offset) return;

		if (optional<Reoriented<PositionInSequence>> const pp = *position(editor.getLocation()))
		{
			Graph const & g = editor.getGraph();
			PlayerJoint const j = closest_joint.first;
			Position new_pos = editor.current_position();
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
			editor.replace(new_pos);
		}
	}

	void JointEditor::dragAllJoints(Reorientation const transform)
	{
		Reoriented<Location> const loc = editor.getLocation();

		if (!dragTransform || !start_seq) return;

		Graph const & g = editor.getGraph();

		if (loc->howFar == 0)
		{
			if (loc->segment.segment == SegmentNum{0})
				return;
		}
		else if (loc->howFar == 1)
		{
			if (loc->segment.segment == last_segment(g[loc->segment.sequence]))
				return;
		}
		else return;

		vector<Position> v = *start_seq;

		auto const posnums = (loc->howFar == 0
			? PosNum::range(PosNum{0}, to(loc->segment).position)
			: PosNum::range(to(loc->segment).position, end(g[loc->segment.sequence])));

		foreach (p : posnums)
			v[p.index] = apply(transform, v[p.index]);

		editor.replace_sequence(v);
	}

}
