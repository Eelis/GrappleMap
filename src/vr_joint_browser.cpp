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
			vector<ReorientedSegment> const candidates = closeCandidates(g, root, nullptr, selection)[j];

			struct Best { Reoriented<Location> loc; double score; };
			optional<Best> best;

			foreach (cand : closeCandidates(g, root, nullptr, selection)[j])
			{
				V3 const
					rayOrigin = at(from_pos(cand), g)[j],
					rayTarget = at(to_pos(cand), g)[j],
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

			if (!best) return boost::none;
			return best->loc;
		}
	}

	void JointBrowser::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		joint = closest_joint(
			editor.current_position(),
			v3(cbData->currentTransformation.getTranslation()),
			0.3);
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
