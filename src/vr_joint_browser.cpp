// todo: rename

#include "vr_editor.hpp"
#include "vr_util.hpp"

namespace GrappleMap
{
	namespace
	{
		optional<Reoriented<Location>> closerLocation(
			Graph const & g,
			V3 const cursor,
			ReorientedSegment const root,
			PlayerJoint const j,
			OrientedPath const * const selection)
		{
			vector<ReorientedSegment> candidates = neighbours(root, g, true);

			candidates.push_back(root);

			struct Best { Reoriented<Location> location; double dist; };
			optional<Best> best;

			foreach (cand : candidates)
			{
				if (selection && !elem(cand->sequence, *selection)) continue;

				V3 const
					rayOrigin = at(from_pos(cand), g)[j],
					rayTarget = at(to_pos(cand), g)[j],
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

	void JointBrowser::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		auto cj = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);

		if (cj && /*jointDefs[cj->joint].draggable &&*/ editor.getViables()[*cj].total_dist != 0)
			joint = *cj;
		else
			joint = boost::none;
	}

	void JointBrowser::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!joint) return;

		OrientedPath const
			tempSel{forwardStep(sequence(segment(editor.getLocation())))},
			& sel = editor.getSelection().empty() ? tempSel : editor.getSelection();

		if (auto l = closerLocation(
				editor.getGraph(),
				v3(cbData->currentTransformation.getTranslation()),
				segment(editor.getLocation()),
				*joint,
				editor.lockedToSelection() ? &sel : nullptr))
		{
			editor.setLocation(*l);
		}
	}

	void JointBrowser::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		editor.snapToPos();
	}
}
