#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	void JointBrowser::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * const cbData)
	{
		joint = closest_joint(
			app.editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.3);
	}

	void JointBrowser::dragCallback(Vrui::DraggingTool::DragCallbackData * const cbData)
	{
		if (!joint) return;

		V3 const cursor = v3(cbData->currentTransformation.getTranslation());

		struct Best { Reoriented<Location> loc; double score; };
		optional<Best> best;

		foreach (cand : app.accessibleSegments[*joint])
		{
			V3 const
				rayOrigin = at(from_pos(cand), app.editor.getGraph())[*joint],
				rayTarget = at(to_pos(cand), app.editor.getGraph())[*joint],
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
			app.editor.setLocation(best->loc);
			app.video_sync();
		}
	}

	void JointBrowser::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		if (snapToPos(app.editor)) app.video_sync();
	}
}
