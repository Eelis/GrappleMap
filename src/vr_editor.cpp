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

	void VrApp::on_select_button(Misc::CallbackData *) { editor.toggle_selected(); }
	void VrApp::on_save_button(Misc::CallbackData *) { editor.save(); }
	void VrApp::on_insert_keyframe_button(Misc::CallbackData *) { editor.insert_keyframe(); }
	void VrApp::on_delete_keyframe_button(Misc::CallbackData *) { editor.delete_keyframe(); }
	void VrApp::on_undo_button(Misc::CallbackData *) { editor.undo(); }
	void VrApp::on_mirror_button(Misc::CallbackData *) { editor.mirror(); }
	void VrApp::on_swap_button(Misc::CallbackData *) { editor.swap_players(); }
	void VrApp::on_branch_button(Misc::CallbackData *) { editor.branch(); }

	void VrApp::frame()
	{
		editor.frame(0.03); // todo: get actual time elapsed

		// Vrui::scheduleUpdate(Vrui::getNextAnimationTime()); // todo: what is this?
	}

	void VrApp::on_lock_toggle(GLMotif::ToggleButton::ValueChangedCallbackData * cb)
	{
		editor.toggle_lock(cb->set);
	}

	void VrApp::on_playback_toggle(GLMotif::ToggleButton::ValueChangedCallbackData *)
	{
		editor.toggle_playback();
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, editor(getopts(argc, argv), nullptr)
	{
		style.grid_size = 20;

		GLMotif::PopupMenu * mainMenu = new GLMotif::PopupMenu("MainMenu", Vrui::getWidgetManager());
		mainMenu->setTitle("GrappleMap");

		auto btn = [&](char const * n, decltype(&VrApp::on_save_button) h)
			{
				auto p = new GLMotif::Button(n, mainMenu, n);
				p->getSelectCallbacks().add(this, h);
			};

		auto lockToggle = new GLMotif::ToggleButton("TransitionLockToggle", mainMenu, "Lock");
		lockToggle->setToggle(editor.lockedToSelection());
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
				case 0: jointBrowser.reset(new JointBrowser(*tool, editor)); break;
				case 1: jointEditor.reset(new JointEditor(*tool, editor)); break;
			}
		}
	}

	void VrApp::display(GLContextData &) const
	{
		glEnable(GL_POINT_SMOOTH); // todo: move

		optional<PlayerJoint> browse_joint, edit_joint;

		if (jointBrowser) browse_joint = jointBrowser->joint;
		if (jointEditor) edit_joint = jointEditor->joint;

		if (!editor.playingBack())
			renderScene(
				editor.getGraph(), current_position(editor),
				editor.getViables(), browse_joint, edit_joint,
				editor.getSelection(), style, playerDrawer);
		else
		{
			glEnable(GL_COLOR_MATERIAL);
			setupLights();
			grid(style.grid_color, style.grid_size, style.grid_line_width);
			playerDrawer.drawPlayers(current_position(editor), {}, {});
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
