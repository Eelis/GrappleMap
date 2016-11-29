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
			bool const open)
		{
			vector<ReorientedSegment> candidates = neighbours(root, g, open);

			candidates.push_back(root);

			struct Best { ReorientedLocation location; double dist; };
			optional<Best> best;

			foreach (cand : candidates)
			{
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
		app.browse_joint =
			closest_joint(
				at(app.location, app.graph),
				v3(cbData->currentTransformation.getTranslation()),
				0.1);
	}

	void VrApp::BrowseTool::calcViables()
	{
		foreach (j : playerJoints)
			app.viables[j] = determineViables(app.graph,
				PositionInSequence{
					app.location.location.segment.sequence,
					app.location.location.segment.segment}, // todo: bad
					j, !app.browseMode, nullptr, app.location.reorientation);
	}

	void VrApp::BrowseTool::dragCallback(Vrui::DraggingTool::DragCallbackData * cbData)
	{
		if (app.browse_joint)
			if (auto l = closerLocation(
					app.graph,
					v3(cbData->currentTransformation.getTranslation()),
					segment(app.location),
					*app.browse_joint, app.browseMode))
			{
				if (l->location.segment != app.location.location.segment)
					calcViables();

				app.location = *l;
			}
	}

	void VrApp::BrowseTool::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData *)
	{
		app.location.location.howFar = std::round(app.location.location.howFar);
	}
}
