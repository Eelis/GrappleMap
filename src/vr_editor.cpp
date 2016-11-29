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

	void VrApp::on_save_button(Misc::CallbackData *)
	{
		save(graph, dbFile);
		std::cout << "Saved.\n";
	}

	void VrApp::on_insert_keyframe_button(Misc::CallbackData *)
	{
		push_undo();

		graph.split_segment(location.location);
		location.location.segment.segment += 1;
		location.location.howFar = 0;

		calcViables();
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
			swap(p[0], p[1]);
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

	void VrApp::on_browse_toggle(GLMotif::ToggleButton::ValueChangedCallbackData * cb)
	{
		browseMode = cb->set;
	}

	void VrApp::calcViables()
	{
		foreach (j : playerJoints)
			viables[j] = determineViables(graph,
				PositionInSequence{
					location.location.segment.sequence,
					location.location.segment.segment}, // todo: bad
					j, !browseMode, nullptr, location.reorientation);
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, programOptions(getopts(argc, argv))
		, dbFile(programOptions["db"].as<std::string>())
		, graph{loadGraph(dbFile)}
	{
		if (auto start = posinseq_by_desc(graph, programOptions["start"].as<string>()))
			location.location.segment = SegmentInSequence{start->sequence, start->position}; // bad
		else
			throw std::runtime_error("no such position/transition");

		GLMotif::PopupMenu * mainMenu = new GLMotif::PopupMenu("MainMenu", Vrui::getWidgetManager());
		mainMenu->setTitle("GrappleMap");

		auto btn = [&](char const * n, decltype(&VrApp::on_save_button) h)
			{
				auto p = new GLMotif::Button(n, mainMenu, n);
				p->getSelectCallbacks().add(this, h);
			};

		auto browseToggle = new GLMotif::ToggleButton("BrowseToggle", mainMenu, "Browse Mode");
		browseToggle->getValueChangedCallbacks().add(this, &VrApp::on_browse_toggle);

		btn("Undo", &VrApp::on_undo_button);
		btn("Mirror", &VrApp::on_mirror_button);
		btn("Branch", &VrApp::on_branch_button);
		btn("Swap", &VrApp::on_swap_button);
		btn("Save", &VrApp::on_save_button);
		btn("Insert Keyframe", &VrApp::on_insert_keyframe_button);

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
		Position const reorientedPosition = at(location, graph);

		Position posToDraw = reorientedPosition;

		glEnable(GL_POINT_SMOOTH);

		glPushMatrix();
		renderScene(graph, posToDraw, viables, browse_joint, edit_joint, !browseMode, location.location.segment.sequence, style);
		glPopMatrix();
	}

	void VrApp::resetNavigation()
	{
		Vrui::NavTransform t=Vrui::NavTransform::identity;
		t *= Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		t *= Vrui::NavTransform::scale(Vrui::getMeterFactor() /* * 0.18 */);
		Vrui::setNavigationTransformation(t);
	}
}

VRUI_APPLICATION_RUN(GrappleMap::VrApp)
