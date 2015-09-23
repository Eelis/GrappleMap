#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <map>
#include <algorithm>

#include <boost/optional.hpp>

#define JOINTS \
	LeftToe, RightToe, \
	LeftHeel, RightHeel, \
	LeftAnkle, RightAnkle, \
	LeftKnee, RightKnee, \
	LeftHip, RightHip, \
	LeftShoulder, RightShoulder, \
	LeftElbow, RightElbow, \
	LeftWrist, RightWrist, \
	LeftHand, RightHand, \
	LeftFingers, RightFingers, \
	Core, Neck, Head

enum Joint: uint32_t { JOINTS };

constexpr Joint joints[] = { JOINTS };

#undef JOINTS

constexpr uint32_t joint_count = sizeof(joints) / sizeof(Joint);

struct PlayerJoint { unsigned player; Joint joint; };

struct V3 { GLdouble x, y, z; };

template<typename T> using PerPlayer = std::array<T, 2>;
template<typename T> using PerJoint = std::array<T, joint_count>;

template<typename T>
struct PerPlayerJoint: PerPlayer<PerJoint<T>>
{
	using PerPlayer<PerJoint<T>>::operator[];

	T & operator[](PlayerJoint i) { return this->operator[](i.player)[i.joint]; }
	T const & operator[](PlayerJoint i) const { return operator[](i.player)[i.joint]; }
};


std::array<PlayerJoint, joint_count * 2> make_playerJoints()
{
	std::array<PlayerJoint, joint_count * 2> r;
	unsigned i = 0;
	for (unsigned player = 0; player != 2; ++player)
		for (auto j : joints)
			r[i++] = {player, j};
	return r;
}

const auto playerJoints = make_playerJoints();

struct Segment
{
	std::array<Joint, 2> ends;
	double length; // in meters
	bool visible;
};

const Segment segments[] =
	{ {{LeftToe, LeftHeel}, 0.25, false}
	, {{LeftToe, LeftAnkle}, 0.18, false}
	, {{LeftHeel, LeftAnkle}, 0.11, false}
	, {{LeftAnkle, LeftKnee}, 0.42, true}
	, {{LeftKnee, LeftHip}, 0.45, true}
	, {{LeftHip, Core}, 0.3, true}
	, {{Core, LeftShoulder}, 0.44, true}
	, {{LeftShoulder, LeftElbow}, 0.29, true}
	, {{LeftElbow, LeftWrist}, 0.26, true}
	, {{LeftWrist, LeftHand}, 0.08, true}
	, {{LeftHand, LeftFingers}, 0.08, true}
	, {{LeftWrist, LeftFingers}, 0.14, false}

	, {{RightToe, RightHeel}, 0.25, false}
	, {{RightToe, RightAnkle}, 0.18, false}
	, {{RightHeel, RightAnkle}, 0.11, false}
	, {{RightAnkle, RightKnee}, 0.42, true}
	, {{RightKnee, RightHip}, 0.45, true}
	, {{RightHip, Core}, 0.3, true}
	, {{Core, RightShoulder}, 0.44, true}
	, {{RightShoulder, RightElbow}, 0.29, true}
	, {{RightElbow, RightWrist}, 0.26, true}
	, {{RightWrist, RightHand}, 0.08, true}
	, {{RightHand, RightFingers}, 0.08, true}
	, {{RightWrist, RightFingers}, 0.14, false}

	, {{LeftShoulder, RightShoulder}, 0.4, false}
	, {{LeftHip, RightHip}, 0.30, true}

	, {{LeftShoulder, Neck}, 0.23, true}
	, {{RightShoulder, Neck}, 0.23, true}
	, {{Neck, Head}, 0.175, true}
	};

struct V2 { GLdouble x, y; };


struct V4
{
	GLdouble x, y, z, w;

	V4(V3 v, GLdouble w)
		: x(v.x), y(v.y), z(v.z), w(w)
	{}
};

V2 xy(V3 v){ return {v.x, v.y}; }
V2 xy(V4 v){ return {v.x, v.y}; }
V3 xyz(V4 v){ return {v.x, v.y, v.z}; }

using M = std::array<double, 16>;

M perspective(double fovy, double aspect, double zNear, double zFar)
{
	auto f = 1/tan(fovy*M_PI/360);

	return
		{ f/aspect, 0, 0, 0
		, 0, f, 0, 0
		, 0, 0, (zFar+zNear)/(zNear-zFar), -1
		, 0, 0, 2*zFar*zNear/(zNear-zFar), 0 };
}

double norm2(V2 v){ return v.x * v.x + v.y * v.y; }
double norm2(V3 v){ return v.x * v.x + v.y * v.y + v.z * v.z; }
double norm(V3 v){ return sqrt(norm2(v)); }

V2 operator-(V2 a, V2 b) { return {a.x - b.x, a.y - b.y}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator-(V3 v) { return {-v.x, -v.y, -v.z}; }
V4 operator-(V4 a, V4 b) { return {{a.x - b.x, a.y - b.y, a.z - b.z}, a.w - b.w}; }
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V4 operator+(V4 a, V4 b) { return {{a.x + b.x, a.y + b.y, a.z + b.z}, a.w + b.w}; }
V3 operator*(V3 v, double s) { return {v.x * s, v.y * s, v.z * s}; }
V3 & operator+=(V3 & a, V3 b) { return a = a + b; }
V3 & operator-=(V3 & a, V3 b) { return a = a - b; }
V4 & operator-=(V4 & a, V4 b) { return a = a - b; }

std::ostream & operator<<(std::ostream & o, V3 v){ return o << '{' << v.x << ',' << v.y << ',' << v.z << '}'; }

V2 operator/(V2 v, double s) { return {v.x / s, v.y / s}; }
V3 operator/(V3 v, double s) { return {v.x / s, v.y / s, v.z / s}; }
V3 normalize(V3 v){ return v / norm(v); }

inline void glVertex(V3 const & v) { glVertex3d(v.x, v.y, v.z); }
inline void glTranslate(V3 const & v) { glTranslated(v.x, v.y, v.z); }

struct Player
{
	std::array<V3, joint_count> joints;

	Player()
	{
		for (auto && j : joints) j = {double((rand()%6)-3), double((rand()%6)), double((rand()%6)-3)};
	}
};

double distance(V3 from, V3 to)
{
	return norm(to - from);
}

Player spring(Player const & p)
{
	Player r;

	for (auto && j : joints)
	{
		r.joints[j] = p.joints[j];

		for (auto && s : segments)
		{
			if (s.ends[0] == j)
			{
				double force = (s.length - distance(p.joints[s.ends[1]], p.joints[s.ends[0]])) / 5;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.ends[1]] - p.joints[s.ends[0]]);
					r.joints[j] -= dir * force;
				}
			}
			else if (s.ends[1] == j)
			{
				double force = (s.length - distance(p.joints[s.ends[1]], p.joints[s.ends[0]])) / 5;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.ends[0]] - p.joints[s.ends[1]]);
					r.joints[j] -= dir * force;
				}
			}
		}

		r.joints[j].y = std::max(0., r.joints[j].y);
	}

	return r;
}

struct Position
{
	Player players[2];

	V3 & operator[](PlayerJoint pj) { return players[pj.player].joints[pj.joint]; }
	V3 const & operator[](PlayerJoint pj) const { return players[pj.player].joints[pj.joint]; }
};

void save(std::string const filename, std::vector<Position> const & v)
{
	std::ofstream f(filename, std::ios::binary);

	std::copy_n(reinterpret_cast<char const *>(v.data()), v.size() * sizeof(Position), std::ostreambuf_iterator<char>(f));
}

std::vector<Position> load(std::string const filename)
{
	std::ifstream f(filename, std::ios::binary);
	std::istreambuf_iterator<char> i(f), e;
	std::string s(i, e);

	size_t const n = s.size() / sizeof(Position);

	std::vector<Position> r(n);
	std::copy(s.begin(), s.end(), reinterpret_cast<char *>(r.data())); // TODO: don't be evil
	return r;
}

void spring(Position & pos)
{
	for (auto && p : pos.players) p = spring(p);
}

void triangle(V3 a, V3 b, V3 c)
{
	glVertex(a);
	glVertex(b);
	glVertex(c);
}

void render(Player const & player, Segment const s)
{
	glBegin(GL_LINES);
		glVertex(player.joints[s.ends[0]]);
		glVertex(player.joints[s.ends[1]]);
	glEnd();
}

void renderWires(Player const & player)
{
	glLineWidth(50);

	for (auto && s : segments) if (s.visible) render(player, s);
}

void renderShape(Player const & player)
{
	auto && j = player.joints;

	auto t = [&](Joint a, Joint b, Joint c)
		{ triangle(j[a], j[b], j[c]); };

	auto crotch = (j[LeftHip] + j[RightHip]) / 2;

	glBegin(GL_TRIANGLES);
		t(LeftAnkle, LeftToe, LeftHeel);
		t(RightAnkle, RightToe, RightHeel);
		t(LeftHip, RightHip, Core);
		t(Core, LeftShoulder, Neck);
		t(Core, RightShoulder, Neck);
		triangle(j[LeftKnee], crotch, j[LeftHip]);
		triangle(j[RightKnee], crotch, j[RightHip]);
	glEnd();

	glPushMatrix();
		glTranslate(player.joints[Head]);
		GLUquadricObj * Sphere = gluNewQuadric();
		gluSphere(Sphere, 0.125, 20, 20);
		gluDeleteQuadric(Sphere);
	glPopMatrix();
}

V3 const red{1,0,0}, blue{0,0,1}, grey{0.2, 0.2, 0.2}, yellow{1,1,0}, green{0,1,0};

inline void glColor(V3 v) { glColor3d(v.x, v.y, v.z); }

void render(Position const & pos, V3 acolor, V3 bcolor, bool ghost = false)
{
	Player const & a = pos.players[0], & b = pos.players[1];

	glColor(acolor);
	if (!ghost) renderShape(a);
	renderWires(a);

	glColor(bcolor);
	if (!ghost) renderShape(b);
	renderWires(b);
}

using Moves = PerPlayerJoint<std::map<double /* distance */, unsigned /* position */>>;

// state

std::vector<Position> positions = load("positions.dat");
unsigned current_pos = 0; // index into positions
V3 camera{0, -0.5, 1.5};
	// x used for rotation over y axis, y used for rotation over x axis, z used for zoom
PlayerJoint closest_joint = {0, LeftAnkle};
boost::optional<PlayerJoint> chosen_joint;
GLFWwindow * window;
Moves candidates;

V4 operator*(M const & m, V4 v)
{
	return
		{{ m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]
		, m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]
		, m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]}
		, m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]};
}

void renderDiff(Player const & a, Player const & b)
{
	glColor3f(0.5, 0.5, 0.5);
	glLineWidth(2);
	glBegin(GL_LINES);

	for (auto j : joints)
	{
		glVertex(a.joints[j]);
		glVertex(b.joints[j]);
	}

	glEnd();
}

void renderDiff(Position const & a, Position const & b)
{
	for (int i = 0; i != 2; ++i)
		renderDiff(a.players[i], b.players[i]);
}

void grid()
{
	glColor3f(0.5,0.5,0.5);
	glLineWidth(2);
	glBegin(GL_LINES);
		for (int i = -4; i <= 4; ++i)
		{
			glVertex3f(i, 0, -4);
			glVertex3f(i, 0, 4);
			glVertex3f(-4, 0, i);
			glVertex3f(4, 0, i);
		}
	glEnd();
}

M yrot(double a)
{
	return
		{ cos(a), 0, sin(a), 0
		, 0, 1, 0, 0
		, -sin(a), 0, cos(a), 0
		, 0, 0, 0, 1 };
}

M xrot(double a)
{
	return
		{ 1, 0, 0, 0
		, 0, cos(a), -sin(a), 0
		, 0, sin(a), cos(a), 0
		, 0, 0, 0, 1 };
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		switch (key)
		{
			case GLFW_KEY_INSERT:
			{
				auto p = positions[current_pos];
				positions.push_back(p);
				current_pos = positions.size() - 1;
			}

			case GLFW_KEY_S:
			{
				save("positions.dat", positions);
				break;
			}
			case GLFW_KEY_P:
			{
				--current_pos %= positions.size();
				break;
			}
			case GLFW_KEY_N:
			{
				++current_pos %= positions.size();
				break;
			}
			case GLFW_KEY_DELETE:
			{
				positions.erase(positions.begin() + current_pos);
				break;
			}
		}
}

double dist(Player const & a, Player const & b)
{
	double d = 0;
	for (auto && j : joints)
		d += distance(a.joints[j], b.joints[j]);
	return d;
}

double dist(Position const & a, Position const & b)
{
	return dist(a.players[0], b.players[0]) + dist(a.players[1], b.players[1]);
}

double dist_via(Position const & a, Position const & b, PlayerJoint pj)
{
	return dist(a, b) + norm2(b[pj] - a[pj]) * 10;
}

constexpr unsigned candidates_shown = 10; // todo: define keys for increasing/decreasing

void drawCandidates()
{
	glColor3f(0.5,0.5,0.5);
	glLineWidth(2);
	glBegin(GL_LINES);
		for (auto pj : playerJoints)
		{
			unsigned i = 0;
			for (auto && p : candidates[pj])
				if (++i == candidates_shown) break;
				else
				{
					glVertex(positions[current_pos][pj]);
					glVertex(positions[p.second][pj]);
				}
		}
	glEnd();
}

void drawClosestJoint()
{
	glColor3f(0,1,0);
	glPointSize(20);
	glBegin(GL_POINTS);
	glVertex(positions[current_pos][closest_joint]);
	glEnd();
}

void determineCandidates()
{
	for (auto pj : playerJoints)
		for (unsigned pos = 0; pos != positions.size(); ++pos)
			candidates[pj][dist_via(positions[current_pos], positions[pos], pj)] = pos;
}

template<typename F>
void determineNearestJoint(F distance_to_cursor)
{
	double closest = 100;

	for (int i = 0; i != 2; ++i)
	{
		auto & player = positions[current_pos].players[i];

		for (auto && ji : joints)
		{
			V3 j = player.joints[ji];

			double d = distance_to_cursor(j);

			if (d < closest)
			{
				closest = d;
				closest_joint.joint = ji;
				closest_joint.player = i;
			}
		}
	}
}

template<typename F>
void explore(F distance_to_cursor)
{
	if (chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		double best = 1000000;

		unsigned u = 0;

		for (auto && candidate : candidates[*chosen_joint])
		{
			if (++u == candidates_shown) break;

			double d = distance_to_cursor(positions[candidate.second][*chosen_joint]);

			if (d < best)
			{
				best = d;
				current_pos = candidate.second;
			}
		}
	}
}

void prepareDraw(M const & persp, int width, int height)
{
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixd(persp.data());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0, 0, -camera.z);
	glMultMatrixd(xrot(camera.y).data());
	glMultMatrixd(yrot(camera.x).data());
}

void draw()
{
	grid();
	render(positions[current_pos], red, blue);

	glDisable(GL_DEPTH);
	glDisable(GL_DEPTH_TEST);

	drawCandidates();
	drawClosestJoint();
}

V2 world2xy(V3 v, M const & persp)
{
	auto v4 = V4(v, 1);
	v4 = yrot(camera.x) * v4;
	v4 = xrot(camera.y) * v4;
	v4.z -= camera.z;
	v4 = persp * v4;
	return xy(v4) / v4.w;
};

int main()
{
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwSetMouseButtonCallback(window, [](GLFWwindow *, int button, int action, int mods)
		{
			if (action == GLFW_PRESS) chosen_joint = closest_joint;
			if (action == GLFW_RELEASE) chosen_joint = boost::none;
		});
	
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.y -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.y += 0.05;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.x -= 0.02;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.x += 0.02;
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.z -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.z += 0.05;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		float const ratio = width / (float) height;

		auto const persp = perspective(90, ratio, 0.1, 10);

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		V2 const cursor {((xpos / width) - 0.5) * 2, ((1-(ypos / height)) - 0.5) * 2};

		auto distance_to_cursor = [&](V3 v){ return norm2(world2xy(v, persp) - cursor); };

		determineCandidates();

		// editing

		if (chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			Position new_pos = positions[current_pos];

			V4 const dragger = yrot(-camera.x) * V4{{1,0,0},0};

			auto & joint = new_pos[*chosen_joint];

			auto off = world2xy(joint, persp) - cursor;

			joint.x -= dragger.x * off.x;
			joint.z -= dragger.z * off.x;
			joint.y = std::max(0., joint.y - off.y);

			spring(new_pos);

			positions[current_pos] = new_pos;
		}

		explore(distance_to_cursor);

		determineNearestJoint(distance_to_cursor);
		
		prepareDraw(persp, width, height);
		draw();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}
