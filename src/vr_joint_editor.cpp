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

	void JointEditor::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData * cb)
	{
		Graph const & g = editor.getGraph();

		editor.push_undo();

		{
			vector<Position> v;
			foreach (p : positions(g, sequence(segment(editor.getLocation()))))
				v.push_back(at(p, g));
			start_seq.emplace(v);
		}

		dragTransform = inverse(reorientationTransformation(cb->startTransformation));

		if (joint)
			joint_edit_offset.emplace(v3(cb->startTransformation.getTranslation()) -
				editor.current_position()[*joint]);
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
		Reoriented<Location> const loc = editor.getLocation();

		optional<Reoriented<PositionInSequence>> const
			pp = position(editor.getLocation());

		if (!pp || !dragTransform || !start_seq) return;

		Graph const & g = editor.getGraph();
		Position new_pos = editor.current_position();
		auto const cursor = v3(cbData->currentTransformation.getTranslation());

		Reorientation const transform
			= compose(*dragTransform, reorientationTransformation(cbData->currentTransformation));

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
			editor.replace(new_pos);
		}
		else
		{
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
}
