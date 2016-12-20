#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void JointBrowser::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * const cbData)
	{
		joint = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.3);
	}

	void JointBrowser::dragCallback(Vrui::DraggingTool::DragCallbackData * const cbData)
	{
		if (!joint) return;

		V3 const cursor = v3(cbData->currentTransformation.getTranslation());

		struct Best { Reoriented<Location> loc; double score; };
		optional<Best> best;

		foreach (cand : accessibleSegments[*joint])
		{
			V3 const
				rayOrigin = at(from_pos(cand), editor.getGraph())[*joint],
				rayTarget = at(to_pos(cand), editor.getGraph())[*joint],
				rayDir = rayTarget - rayOrigin;

			double c = closest(rayOrigin, rayDir, cursor);

			if (c < 0) c = 0;
			if (c > 1) c = 1;

			if (0 <= c && c <= 1)
			{
				double const score = distance(cursor, rayOrigin + rayDir * c);

				if (!best || score < best->score)
					best = Best{loc(cand, c), score};
			}
		}

		if (best)
		{
			editor.setLocation(best->loc);
			seek();
		}
	}

	void JointBrowser::seek() const
	{
		if (!video_player) return;

		if (auto t = timeInSelection(editor))
			video_player->gotoRecordedFrame(*t);
	}

	void JointBrowser::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		if (snapToPos(editor)) seek();
	}
}
