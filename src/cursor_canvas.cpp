#include "editor_canvas.hpp"

using namespace GrappleMap;

namespace
{
	struct Application: EditorCanvas
	{
		std::deque<Reoriented<Reversible<SegmentInSequence>>> queue;
		double howFar = 0;
		Position chaser;

		Position displayPos() const override
		{
			return chaser;
		}

		bool renderPlain() const override { return true; }

		void progress(double elapsed) override
		{
			if (queue.empty()) return;

			if (queue.size() > 10)
			{
				elapsed *= (queue.size() - 10.)/2;
			}

			while (elapsed > 0)
			{
				bool detailed = false;

				double remaining = (1 - howFar) / 5;

				if (remaining > elapsed)
				{
					howFar += elapsed * 5;
					break;
				}

				howFar = 1;
				elapsed -= remaining;

				if (queue.size() == 1) break;

				queue.pop_front();
				howFar = 0;
			}

			double x = queue.front()->reverse ? 1 - howFar : howFar;

			Reoriented<Location> const rl = Location{**queue.front(), x} * queue.front().reorientation;

			chaser = between(chaser, at(rl, editor.getGraph()), 0.2);
		}
	};
}

EMSCRIPTEN_BINDINGS(GrappleMap_cursor_canvas)
{
	emscripten::function("cursor_canvas_main", +[]
		{
			app.reset(new Application);
			return to_elaborate_jsval(app->editor.getGraph());
		});

	emscripten::function("cursor_canvas_goto", +[](uint16_t nodeid) -> bool
	{
		Graph const & g = app->editor.getGraph();

		Application & w = *static_cast<Application *>(app.get()); // TODO: gross

		if (optional<Reoriented<Step>> const step = go_to(NodeNum{nodeid}, app->editor))
			foreach (sis : segments(*step, g))
				w.queue.push_back(sis);
		else
		{
			w.queue.clear();
			auto const seg = segment(app->editor.getLocation());
			w.queue.push_back(Reoriented<Reversible<SegmentInSequence>>{
				{*seg, false},
				seg.reorientation});

			Reoriented<Location> const rl = Location{*seg, w.howFar} * seg.reorientation;

			w.chaser = at(rl, app->editor.getGraph());
		}

		return true;
	});
}

