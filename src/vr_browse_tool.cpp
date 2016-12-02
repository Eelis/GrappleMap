#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	namespace
	{
		optional<ReorientedLocation> closerLocation(
			Graph const & g,
			V3 const cursor,
			ReorientedSegment const root,
			PlayerJoint const j,
			Selection const * const selection)
		{
			vector<ReorientedSegment> candidates = neighbours(root, g, true);

			candidates.push_back(root);

			struct Best { ReorientedLocation location; double dist; };
			optional<Best> best;

			foreach (cand : candidates)
			{
				if (selection && !elem(cand.segment.sequence, *selection)) continue;

				V3 const
					rayOrigin = at(from(cand), g)[j],
					rayTarget = at(to(cand), g)[j],
					rayDir = rayTarget - rayOrigin;

				if (norm2(rayDir) < 0.05) continue; // tiny segments are not nice for browsing
					// todo: make sure exactly the corresponding viables are shown

				double c = closest(rayOrigin, rayDir, cursor);

				if (c < 0) c = 0;
				if (c > 1) c = 1;

				if (0 <= c && c <= 1)
				{
					double const d = distance(cursor, rayOrigin + rayDir * c);

					if (!best || d < best->dist) best = Best{loc(cand, c), d};
				}
			}

			// todo: can we use a std::minimal thingy here?

			if (!best) return boost::none;

			return best->location;
		}
	}

	void BrowseTool::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		auto cj = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);

		if (cj && /*jointDefs[cj->joint].draggable &&*/ editor.getViables()[*cj].total_dist != 0)
			editor.browse_joint = *cj;
		else
			editor.browse_joint = boost::none;
	}

	void BrowseTool::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!editor.browse_joint) return;

		Selection const
			tempSel{sequence(segment(editor.getLocation()))},
			& sel = editor.getSelection().empty() ? tempSel : editor.getSelection();

		if (auto l = closerLocation(
				editor.getGraph(),
				v3(cbData->currentTransformation.getTranslation()),
				segment(editor.getLocation()),
				*editor.browse_joint,
				editor.lockToTransition ? &sel : nullptr))
		{
			if (l->location.segment != editor.getLocation().location.segment)
				editor.calcViables();

			editor.location = *l;
		}
	}

	void BrowseTool::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		double & c = editor.location.location.howFar;

		double const r = std::round(c);

		if (std::abs(c - r) < 0.2) c = r;
	}
}
