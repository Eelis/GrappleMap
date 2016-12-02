#include "vr_editor.hpp"
#include "graph_util.hpp"
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>

namespace po = boost::program_options;

namespace GrappleMap
{
	namespace
	{
		string const usage =
			"Usage: grapplemap-vr-editor [OPTIONS] [START]\n";

		string const start_desc =
			"START can be:\n"
			"  - \"p#\" for position #\n"
			"  - \"t#\" for transition #\n"
			"  - the first line of a position or transition's description,\n"
			"    with newlines replaced with spaces\n"
			"  - \"last-trans\" for the last transition in the database\n";

		po::variables_map getopts(int argc, char const * const * const argv)
		{
			po::options_description optdesc("Options");
			optdesc.add_options()
				("help,h", "show this help")
				("start", po::value<string>()->default_value("last-trans"), "see START below")
				("db", po::value<string>()->default_value("GrappleMap.txt"), "database file");

			po::positional_options_description posopts;
			posopts.add("start", -1);

			po::variables_map vm;
			po::store(
				po::command_line_parser(argc, argv)
					.options(optdesc)
					.positional(posopts)
					.run(),
				vm);
			po::notify(vm);

			if (vm.count("help"))
			{
				std::cout << usage << '\n' << optdesc << '\n' << start_desc << '\n';
				abort(); // todo: bad
			}

			return vm;
		}
	}

	void VrApp::on_select_button(Misc::CallbackData *)
	{
		SeqNum const curseq = location.location.segment.sequence;

		if (selection.empty()) selection.push_back({curseq, location.reorientation});
		else if (selection.front().sequence == curseq) selection.pop_front();
		else if (selection.back().sequence == curseq) selection.pop_back();
		else if (!elem(curseq, selection))
		{
			if (graph.to(curseq).node == graph.from(selection.front().sequence).node)
			{
				selection.push_front(sequence(segment(location)));
			}
			else if (graph.from(curseq).node == graph.to(selection.back().sequence).node)
			{
				selection.push_back(sequence(segment(location)));
			}

//			else if (it's after a known one) push_back();
		}

		// todo: redraw
	}

	void VrApp::on_save_button(Misc::CallbackData *)
	{
		save(graph, dbFile);
		std::cout << "Saved.\n";
	}

	void VrApp::on_insert_keyframe_button(Misc::CallbackData *)
	{
		push_undo();

		graph.split_segment(location.location);
		++location.location.segment.segment;
		location.location.howFar = 0;

		calcViables();
	}

	void VrApp::on_delete_keyframe_button(Misc::CallbackData *)
	{
		if (optional<PositionInSequence> const p = position(location.location))
		{
			push_undo();

			if (auto const new_pos = graph.erase(*p))
			{
				//todo: location.position = *new_pos;
				calcViables();
			}
			else undo.pop();
		}
	}

	void VrApp::on_undo_button(Misc::CallbackData *)
	{
		if (undo.empty()) return;

		std::tie(graph, location) = undo.top();
		undo.pop();
	}

	void VrApp::on_mirror_button(Misc::CallbackData *)
	{
		location.reorientation.mirror = !location.reorientation.mirror;
	}

	void VrApp::on_swap_button(Misc::CallbackData *)
	{
		if (auto const pp = position(location.location))
		{
			push_undo();

			auto p = graph[*pp];
			swap_players(p);
			graph.replace(*pp, p, true);
		}
	}

	void VrApp::on_branch_button(Misc::CallbackData *)
	{
		if (auto const pp = position(location.location))
		{
			push_undo();

			try { split_at(graph, *pp); }
			catch (exception const & e)
			{
				cerr << "could not branch: " << e.what() << '\n';
			}
		}
	}

	void VrApp::frame()
	{
		if (playbackLoc)
		{
			auto & hf = playbackLoc->location.howFar;
			
			hf += 0.03; // todo
			if (hf > 1)
			{
				hf -= 1;

				auto & seg = playbackLoc->location.segment;

				if (seg == last_segment(graph, seg.sequence))
					seg.segment.index = 0;
				else
					++seg.segment;
			}

			// Vrui::scheduleUpdate(Vrui::getNextAnimationTime()); // todo: what is this?
		}
	}

	void VrApp::on_lock_toggle(GLMotif::ToggleButton::ValueChangedCallbackData * cb)
	{
		lockToTransition = cb->set;
	}

	void VrApp::on_playback_toggle(GLMotif::ToggleButton::ValueChangedCallbackData *)
	{
		if (playbackLoc) playbackLoc = boost::none;
		else
		{
			if (selection.empty()) selection = {sequence(segment(location))};

			playbackLoc = from(first_segment(selection.front()));
		}
	}

	void VrApp::calcViables()
	{
		foreach (j : playerJoints)
			viables[j] = determineViables(graph, from(location.location.segment), // todo: bad
					j, nullptr, location.reorientation);
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, programOptions(getopts(argc, argv))
		, dbFile(programOptions["db"].as<std::string>())
		, graph{loadGraph(dbFile)}
	{
		if (auto start = posinseq_by_desc(graph, programOptions["start"].as<string>()))
			location.location.segment = segment_from(*start);
		else
			throw std::runtime_error("no such position/transition");

		style.grid_size = 20;

		GLMotif::PopupMenu * mainMenu = new GLMotif::PopupMenu("MainMenu", Vrui::getWidgetManager());
		mainMenu->setTitle("GrappleMap");

		auto btn = [&](char const * n, decltype(&VrApp::on_save_button) h)
			{
				auto p = new GLMotif::Button(n, mainMenu, n);
				p->getSelectCallbacks().add(this, h);
			};

		auto lockToggle = new GLMotif::ToggleButton("TransitionLockToggle", mainMenu, "Lock");
		lockToggle->setToggle(lockToTransition);
		lockToggle->getValueChangedCallbacks().add(this, &VrApp::on_lock_toggle);

		auto playbackToggle = new GLMotif::ToggleButton("PlaybackToggle", mainMenu, "Playback");
		playbackToggle->getValueChangedCallbacks().add(this, &VrApp::on_playback_toggle);

		btn("Undo", &VrApp::on_undo_button);
		btn("Mirror", &VrApp::on_mirror_button);
		btn("Branch", &VrApp::on_branch_button);
		btn("Swap", &VrApp::on_swap_button);
		btn("Save", &VrApp::on_save_button);
		btn("Insert Keyframe", &VrApp::on_insert_keyframe_button);
		btn("Delete Keyframe", &VrApp::on_delete_keyframe_button);
		btn("Select/Unselect Transition", &VrApp::on_select_button);

		mainMenu->manageMenu();
		Vrui::setMainMenu(mainMenu);
	}

	void VrApp::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData * cbData)
	{
		static int tools_created = 0; // horrible hack until i make proper tools

		if (Vrui::DraggingTool * tool = dynamic_cast<Vrui::DraggingTool *>(cbData->tool))
		{
			switch (tools_created++)
			{
				case 0: new BrowseTool(*tool, *this); break;
				case 1: new JointEditor(*tool, *this); break;
					// todo: don't leak
			}
		}
	}

	void VrApp::display(GLContextData &) const
	{
		glEnable(GL_POINT_SMOOTH); // todo: move

		if (!playbackLoc)
			renderScene(
				graph, at(location, graph),
				viables, browse_joint, edit_joint,
				selection, style, playerDrawer);
		else
		{
			glEnable(GL_COLOR_MATERIAL);
			setupLights();
			grid(style.grid_color, style.grid_size, style.grid_line_width);
			playerDrawer.drawPlayers(at(*playbackLoc, graph), {}, {});
		}
	}

	void VrApp::resetNavigation()
	{
		Vrui::NavTransform t=Vrui::NavTransform::identity;
		t *= Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		t *= Vrui::NavTransform::scale(Vrui::getMeterFactor());
		Vrui::setNavigationTransformation(t);
	}
}

VRUI_APPLICATION_RUN(GrappleMap::VrApp)
