#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>

#include "camera.hpp"
#include "editor.hpp"
#include "persistence.hpp"
#include "math.hpp"
#include "positions.hpp"
#include "viables.hpp"
#include "rendering.hpp"
#include "graph_util.hpp"
#include "metadata.hpp"
#include "js_conversions.hpp"
#include <GLFW/glfw3.h>
#include <boost/program_options.hpp>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <algorithm>
#include <codecvt>
#include <iterator>
#include <stack>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GrappleMap
{
	struct Dirty
	{
		vector<NodeNum> nodes_added, nodes_changed;
		vector<SeqNum> edges_added, edges_changed;
	};

	struct EditorCanvas
	{
		EditorCanvas();

		GLuint program;
		GLint mvp_location;
		GLuint ViewMatrixID;
		GLuint ModelMatrixID;
		GLuint LightEnabledLoc;
		GLuint vertex_buffer, vertex_shader, fragment_shader;
		GLint vpos_location, vcol_location, norm_location;

		vector<BasicVertex> vertices;
		double lastTime{};

		PlayerJoint closest_joint = {{0}, LeftAnkle};
		optional<PlayerJoint> chosen_joint;
		bool edit_mode = false;
		optional<Position> clipboard;
		Camera camera;
		Editor editor{loadGraph("GrappleMap.txt")};
		double jiggle = 0;
		optional<V2> cursor;
		Style style;
		PlayerDrawer playerDrawer;
		PerPlayerJoint<vector<Reoriented<SegmentInSequence>>> candidates;
		int videoOffset = 0;
		GLFWwindow * window;
		string joints_to_edit = "single_joint";
		bool confine_horizontal = false;
		bool confine_interpolation = false;
		bool confine_local_edits = true;
		bool transform_rotate = false;
		Dirty dirty;
		bool ignore_keyboard = false;

		vector<View> const & views() const;

		virtual void progress(double elapsed);
		virtual bool renderPlain() const { return bool(editor.playingBack()); }
		virtual Position displayPos() const;

		void frame();
		size_t makeVertices();

		virtual ~EditorCanvas() {}
	};

	extern unique_ptr<EditorCanvas> app;
}
