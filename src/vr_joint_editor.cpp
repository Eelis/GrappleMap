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

		VruiRotation y_only(VruiRotation const & r)
		{
			Geometry::Vector<double, 3> v(1,0,0);
			auto w = r.transform(v);
			double a = std::atan2(w[0], w[2]);
			return VruiRotation::fromEulerAngles(0, a, 0);
		}

		VruiVector xz_only(VruiVector const v)
		{
			return {v[0], 0, v[2]};
		}

		ONTransform reorientationTransformation(
			Geometry::OrthogonalTransformation<double, 3> const & t)
		{
			return ONTransform(xz_only(t.getTranslation()), y_only(t.getRotation()));
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

		dragTransform = reorientationTransformation(cb->startTransformation);
		dragTransform->doInvert();

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

		ONTransform transform = reorientationTransformation(cbData->currentTransformation);
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
			editor.replace(new_pos);
		}
		else
		{
			vector<Position> v = *start_seq;

			if (loc->howFar == 0)
				foreach (p : PosNum::range(PosNum{0}, to(loc->segment).position))
					v[p.index] = v[p.index] * transform;
			else if (loc->howFar == 1)
				foreach (p : PosNum::range(to(loc->segment).position, end(g[loc->segment.sequence])))
					v[p.index] = v[p.index] * transform;
			else return;

			editor.replace_sequence(v);
		}
	}
}
