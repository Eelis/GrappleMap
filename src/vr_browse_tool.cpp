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

	void VrApp::BrowseTool::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData * cbData)
	{
		auto cj = closest_joint(
			at(app.location, app.graph),
			v3(cbData->currentTransformation.getTranslation()),
			0.1);

		if (cj && /*jointDefs[cj->joint].draggable &&*/ app.viables[*cj].total_dist != 0)
			app.browse_joint = *cj;
		else
			app.browse_joint = boost::none;
	}

	void VrApp::BrowseTool::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (!app.browse_joint) return;

		Selection const
			tempSel{sequence(segment(app.location))},
			& sel = app.selection.empty() ? tempSel : app.selection;

		if (auto l = closerLocation(
				app.graph,
				v3(cbData->currentTransformation.getTranslation()),
				segment(app.location),
				*app.browse_joint,
				app.lockToTransition ? &sel : nullptr))
		{
			if (l->location.segment != app.location.location.segment)
				app.calcViables();

			app.location = *l;
		}
	}

	void VrApp::BrowseTool::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		double & c = app.location.location.howFar;

		double const r = std::round(c);

		if (std::abs(c - r) < 0.2) c = r;
	}
}
