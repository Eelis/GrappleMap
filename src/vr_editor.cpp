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
				("db", po::value<string>()->default_value("GrappleMap.txt"), "database file")
				("scale", po::value<double>()->default_value(1.0));

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

	void VrApp::on_lock_toggle(ToggleEvent * cb) { editor.toggle_lock(cb->set); }
	void VrApp::on_playback_toggle(ToggleEvent *) { editor.toggle_playback(); }
	void VrApp::on_select_button(Misc::CallbackData *) { editor.toggle_selected(); }
	void VrApp::on_save_button(Misc::CallbackData *) { editor.save(); }
	void VrApp::on_insert_keyframe_button(Misc::CallbackData *) { editor.insert_keyframe(); }
	void VrApp::on_delete_keyframe_button(Misc::CallbackData *) { editor.delete_keyframe(); }
	void VrApp::on_undo_button(Misc::CallbackData *) { editor.undo(); }
	void VrApp::on_mirror_button(Misc::CallbackData *) { editor.mirror(); }
	void VrApp::on_swap_button(Misc::CallbackData *) { swap_players(editor); }
	void VrApp::on_branch_button(Misc::CallbackData *) { editor.branch(); }

	void VrApp::frame()
	{
		video_player.frame();

		{
			viables.clear();

			auto add_vias = [&](PlayerJoint const j)
				{
					viables += determineViables(editor.getGraph(),
						from_pos(segment(editor.getLocation())),
						j, nullptr);
				};

			if (jointBrowser && jointBrowser->joint) add_vias(*(jointBrowser->joint));
			if (jointEditor && jointEditor->joint) add_vias(*(jointBrowser->joint));
		}

		accessibleSegments = closeCandidates(
			editor.getGraph(), segment(editor.getLocation()), nullptr,
			editor.lockedToSelection() ? &editor.getSelection() : nullptr);

		editor.frame(Vrui::getCurrentFrameTime());

		// Vrui::scheduleUpdate(Vrui::getNextAnimationTime()); // todo: what is this?
	}

	void VrApp::on_confine_edits_toggle(ToggleEvent * e)
	{
		confineEdits = e->set;
		if (jointEditor) jointEditor->confined = confineEdits;
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, opts(getopts(argc, argv))
		, editor(opts)
		, scale(opts["scale"].as<double>())
		, video_player(std::vector<std::string>{"vruixine", "test.mp4"})
	{
		style.grid_size = 20;

		GLMotif::PopupMenu * mainMenu = new GLMotif::PopupMenu("MainMenu", Vrui::getWidgetManager());
		mainMenu->setTitle("GrappleMap");

		auto btn = [&](char const * n, decltype(&VrApp::on_save_button) h)
			{
				auto p = new GLMotif::Button(n, mainMenu, n);
				p->getSelectCallbacks().add(this, h);
			};

		auto toggle = [&](char const * n, decltype(&VrApp::on_lock_toggle) h, bool b)
			{
				auto t = new GLMotif::ToggleButton(n, mainMenu, n);
				t->setToggle(b);
				t->getValueChangedCallbacks().add(this, h);
			};

		toggle("Lock", &VrApp::on_lock_toggle, editor.lockedToSelection());
		toggle("Playback", &VrApp::on_playback_toggle, editor.playingBack());
		toggle("Confine Edits", &VrApp::on_confine_edits_toggle, confineEdits);

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
				case 0: jointBrowser.reset(new JointBrowser(*tool, editor)); break;
				case 1: jointEditor.reset(new JointEditor(*tool, editor, confineEdits)); break;
			}
		}
	}

	void VrApp::display(GLContextData & contextData) const
	{
		video_player.display(contextData, this);

		glEnable(GL_POINT_SMOOTH); // todo: move

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		optional<PlayerJoint> browse_joint, edit_joint;

		if (jointBrowser) browse_joint = jointBrowser->joint;
		if (jointEditor) edit_joint = jointEditor->joint;

		if (!editor.playingBack())
			renderScene(
				editor.getGraph(), editor.current_position(),
				viables, browse_joint, edit_joint,
				editor.getSelection(),
				accessibleSegments,
				editor.getLocation()->segment,
				style, playerDrawer);
		else
		{
			glEnable(GL_COLOR_MATERIAL);
			setupLights();
			grid(style.grid_color, style.grid_size, style.grid_line_width);
			playerDrawer.drawPlayers(editor.current_position(), {}, {});
		}
	}

	void VrApp::resetNavigation()
	{
		Vrui::NavTransform t=Vrui::NavTransform::identity;
		t *= Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		t *= Vrui::NavTransform::scale(Vrui::getMeterFactor() * scale);
		Vrui::setNavigationTransformation(t);
	}
}

VRUI_APPLICATION_RUN(GrappleMap::VrApp)
