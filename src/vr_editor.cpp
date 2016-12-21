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
				("video", po::value<string>(), "video file")
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

		vector<PlayerJoint> editedJoints(JointEditor const & e)
		{
			vector<PlayerJoint> r;

			if (e.draggingSingleJoint())
				r.push_back(e.closest_joint.first);
			else if (e.draggingAllJoints())
				foreach (j : playerJoints) r.push_back(j);

			return r;
		}
	}

	void VrApp::video_sync()
	{
		if (videoSyncToggle->getToggle() && video_player)
			if (auto t = timeInSelection(editor))
				video_player->seek(*t);
	}

	void VrApp::on_lock_toggle(ToggleEvent * cb) { editor.toggle_lock(cb->set); }
	void VrApp::on_playback_toggle(ToggleEvent *) { editor.toggle_playback(); }

	void VrApp::on_select_button(Misc::CallbackData *)
	{
		editor.toggle_selected();
		// todo: (re)sync video
	}

	void VrApp::on_save_button(Misc::CallbackData *) { editor.save(); }
	void VrApp::on_delete_keyframe_button(Misc::CallbackData *) { editor.delete_keyframe(); video_sync(); }
	void VrApp::on_undo_button(Misc::CallbackData *) { editor.undo(); video_sync(); }
	void VrApp::on_mirror_button(Misc::CallbackData *) { editor.mirror(); }
	void VrApp::on_swap_button(Misc::CallbackData *) { swap_players(editor); }
	void VrApp::on_branch_button(Misc::CallbackData *) { editor.branch(); }

	void VrApp::on_interpolate_button(Misc::CallbackData *)
	{
		editor.push_undo();

		optional<Reoriented<PositionInSequence>> const pos
			= position(editor.getLocation());
		if (!pos) return;

		Graph const & g = editor.getGraph();

		if (auto nextLoc = next(*pos, g))
		if (auto prevLoc = prev(*pos))
		{
			auto p = between(at(*prevLoc, g), at(*nextLoc, g));
			for(int i = 0; i != 30; ++i) spring(p);
			editor.replace(p);
		}
	}

	void VrApp::on_sync_video_toggle(ToggleEvent * cb)
	{
		if (video_player)
			video_player->setTimeRef(cb->set ? timeInSelection(editor) : boost::none);
	}

	void VrApp::frame()
	{
		if (video_player) video_player->frame();

		{
			viables.clear();

			auto add_vias = [&](PlayerJoint const j)
				{
					viables += determineViables(editor.getGraph(),
						from_pos(segment(editor.getLocation())),
						j, nullptr);
				};

			if (jointBrowser && jointBrowser->joint) add_vias(*(jointBrowser->joint));
			if (jointEditor && jointEditor->draggingSingleJoint())
				add_vias(jointEditor->closest_joint.first);
		}

		accessibleSegments = closeCandidates(
			editor.getGraph(), segment(editor.getLocation()), nullptr,
			editor.lockedToSelection() ? &editor.getSelection() : nullptr);

		editor.frame(Vrui::getCurrentFrameTime());

		if (video_player && editor.playingBack())
			if (auto t = timeInSelection(editor))
				video_player->seek(*t);

		// Vrui::scheduleUpdate(Vrui::getNextAnimationTime()); // todo: what is this?
	}

	void VrApp::on_confine_edits_toggle(ToggleEvent * e)
	{
		confineEdits = e->set;
		if (jointEditor) jointEditor->confined = confineEdits;
	}

	void VrApp::on_keyframe_insertion_enabled_toggle(ToggleEvent * e)
	{
		keyframeInsertionEnabled = e->set;
		if (jointEditor) jointEditor->keyframeInsertionEnabled = keyframeInsertionEnabled;
	}

	VrApp::VrApp(int argc, char ** argv)
		: Vrui::Application(argc, argv)
		, opts(getopts(argc, argv))
		, editor(opts)
		, scale(opts["scale"].as<double>())
		, video_player(opts.count("video")
			? new VruiXine({"vruixine", opts["video"].as<string>()})
			: nullptr)
		, editorControlDialog(createEditorControlDialog())
	{
		style.grid_size = 20;
		Vrui::popupPrimaryWidget(editorControlDialog);
	}

	GLMotif::PopupWindow * VrApp::createEditorControlDialog()
	{
//		const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
		
		GLMotif::PopupWindow * window = new GLMotif::PopupWindow("EditorControlDialogPopup", Vrui::getWidgetManager(),"Editor Control");

		GLMotif::RowColumn * dialog = new GLMotif::RowColumn("EditorControlDialog", window, false);

		auto btn = [&](char const * n, decltype(&VrApp::on_save_button) h)
			{
				auto p = new GLMotif::Button(n, dialog, n);
				p->getSelectCallbacks().add(this, h);
			};

		auto toggle = [&](char const * n, decltype(&VrApp::on_lock_toggle) h, bool b)
			{
				auto t = new GLMotif::ToggleButton(n, dialog, n);
				t->setToggle(b);
				t->getValueChangedCallbacks().add(this, h);
				return t;
			};

		toggle("Playback", &VrApp::on_playback_toggle, bool(editor.playingBack()));
		toggle("Confine browsing to selection", &VrApp::on_lock_toggle, editor.lockedToSelection());
		toggle("Confine edits to interpolations", &VrApp::on_confine_edits_toggle, confineEdits);
		toggle("Enable keyframe insertion", &VrApp::on_keyframe_insertion_enabled_toggle, keyframeInsertionEnabled);
		videoSyncToggle = toggle("Sync Video", &VrApp::on_sync_video_toggle, false);

		btn("Undo edit", &VrApp::on_undo_button);
		btn("Mirror position", &VrApp::on_mirror_button);
		btn("Branch", &VrApp::on_branch_button);
		btn("Swap players", &VrApp::on_swap_button);
		btn("Interpolate position", &VrApp::on_interpolate_button);
		btn("Save database", &VrApp::on_save_button);
		btn("Delete keyframe", &VrApp::on_delete_keyframe_button);
		btn("(Un)select transition", &VrApp::on_select_button);

		dialog->manageChild();
		
		return window;
	}

	void VrApp::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData * cbData)
	{
		static int tools_created = 0; // horrible hack until i make proper tools

		if (Vrui::DraggingTool * tool = dynamic_cast<Vrui::DraggingTool *>(cbData->tool))
		{
			switch (tools_created++)
			{
				case 0: jointBrowser.reset(new JointBrowser(*tool, *this)); break;
				case 1:
					jointEditor.reset(new JointEditor(*tool, editor));
					jointEditor->confined = confineEdits;
					jointEditor->keyframeInsertionEnabled = keyframeInsertionEnabled;
					break;
			}
		}
	}

	void VrApp::display(GLContextData & contextData) const
	{
		if (video_player) video_player->display(contextData, this);

		glEnable(GL_POINT_SMOOTH); // todo: move

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (!editor.playingBack())
		{
			optional<PlayerJoint> browse_joint;
			if (jointBrowser) browse_joint = jointBrowser->joint;

			vector<PlayerJoint> editJoints;
			if (jointEditor) editJoints = editedJoints(*jointEditor);

			renderScene(
				editor.getGraph(), editor.current_position(),
				viables, browse_joint, editJoints,
				editor.getSelection(),
				accessibleSegments,
				editor.getLocation()->segment,
				style, playerDrawer);
		}
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
