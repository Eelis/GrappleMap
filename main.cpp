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

bool operator==(PlayerJoint a, PlayerJoint b)
{
	return a.player == b.player && a.joint == b.joint;
}

struct V3 { GLdouble x, y, z; };

template<typename T> using PerPlayer = std::array<T, 2>;
template<typename T> using PerJoint = std::array<T, joint_count>;

struct JointDef { Joint joint; double radius; };

PerJoint<JointDef> jointDefs =
	{{ { LeftToe, 0.025}
	, { RightToe, 0.025}
	, { LeftHeel, 0.03}
	, { RightHeel, 0.03}
	, { LeftAnkle, 0.03}
	, { RightAnkle, 0.03}
	, { LeftKnee, 0.05}
	, { RightKnee, 0.05}
	, { LeftHip, 0.10}
	, { RightHip, 0.10}
	, { LeftShoulder, 0.08}
	, { RightShoulder, 0.08}
	, { LeftElbow, 0.045}
	, { RightElbow, 0.045}
	, { LeftWrist, 0.02}
	, { RightWrist, 0.02}
	, { LeftHand, 0.02}
	, { RightHand, 0.02}
	, { LeftFingers, 0.02}
	, { RightFingers, 0.02}
	, { Core, 0.02}
	, { Neck, 0.02}
	, { Head, 0.125}
	}};

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

struct PlayerDef { V3 color; };

V3 const red{1,0,0}, blue{0.1, 0.1, 0.9}, grey{0.2, 0.2, 0.2}, yellow{1,1,0}, green{0,1,0};

PerPlayer<PlayerDef> playerDefs = {{ {red}, {blue} }};

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
	, {{LeftKnee, LeftHip}, 0.5, true}
	, {{LeftHip, Core}, 0.3, true}
	, {{Core, LeftShoulder}, 0.4, true}
	, {{LeftShoulder, LeftElbow}, 0.29, true}
	, {{LeftElbow, LeftWrist}, 0.26, true}
	, {{LeftWrist, LeftHand}, 0.08, true}
	, {{LeftHand, LeftFingers}, 0.08, true}
	, {{LeftWrist, LeftFingers}, 0.14, false}

	, {{RightToe, RightHeel}, 0.25, false}
	, {{RightToe, RightAnkle}, 0.18, false}
	, {{RightHeel, RightAnkle}, 0.11, false}
	, {{RightAnkle, RightKnee}, 0.42, true}
	, {{RightKnee, RightHip}, 0.5, true}
	, {{RightHip, Core}, 0.3, true}
	, {{Core, RightShoulder}, 0.4, true}
	, {{RightShoulder, RightElbow}, 0.29, true}
	, {{RightElbow, RightWrist}, 0.26, true}
	, {{RightWrist, RightHand}, 0.08, true}
	, {{RightHand, RightFingers}, 0.08, true}
	, {{RightWrist, RightFingers}, 0.14, false}

	, {{LeftShoulder, RightShoulder}, 0.4, false}
	, {{LeftHip, RightHip}, 0.25, true}

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
				double force = (s.length - distance(p.joints[s.ends[1]], p.joints[s.ends[0]])) / 3;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.ends[1]] - p.joints[s.ends[0]]);
					r.joints[j] -= dir * force;
				}
			}
			else if (s.ends[1] == j)
			{
				double force = (s.length - distance(p.joints[s.ends[1]], p.joints[s.ends[0]])) / 3;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.ends[0]] - p.joints[s.ends[1]]);
					r.joints[j] -= dir * force;
				}
			}
		}

		r.joints[j].y = std::max(jointDefs[j].radius, r.joints[j].y);
	}

	return r;
}

struct Position
{
	Player players[2];

	V3 & operator[](PlayerJoint pj) { return players[pj.player].joints[pj.joint]; }
	V3 const & operator[](PlayerJoint pj) const { return players[pj.player].joints[pj.joint]; }
};

struct PositionOnDisk
{
	unsigned id;
	Position pos;
};

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
}

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

using Moves = PerPlayerJoint<std::map<double /* distance */, std::pair<unsigned /* sequence */, unsigned /* position */>>>;

using Sequence = std::vector<Position>;

std::vector<Sequence> load(std::string const filename)
{
	std::ifstream f(filename, std::ios::binary);
	std::istreambuf_iterator<char> i(f), e;
	std::string s(i, e);

	size_t const n = s.size() / sizeof(PositionOnDisk);

	std::vector<PositionOnDisk> v(n);
	std::copy(s.begin(), s.end(), reinterpret_cast<char *>(v.data())); // TODO: don't be evil

	std::vector<Sequence> r;
	unsigned id = unsigned(-1);

	unsigned total = 0;

	for (auto && pod : v)
	{
		if (pod.id != id)
		{
			r.emplace_back();
			id = pod.id;
		}

		++total;
		r.back().push_back(pod.pos);
	}

	std::cout << "Loaded " << total << " positions in " << r.size() << " sequences.\n";

	if (r.empty()) r.emplace_back(Sequence(1));

	return r;
}

// state

std::vector<Sequence> sequences = load("positions.dat");
unsigned current_sequence = 0;
unsigned current_position = 0; // index into current sequence
V3 camera{0, -0.7, 1.7};
	// x used for rotation over y axis, y used for rotation over x axis, z used for zoom
PlayerJoint closest_joint = {0, LeftAnkle};
boost::optional<PlayerJoint> chosen_joint;
GLFWwindow * window;
Position clipboard;
std::vector<std::pair<unsigned /* sequence */, unsigned /* position */>> candidates;

V4 operator*(M const & m, V4 const v)
{
	return
		{{ m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]
		, m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]
		, m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]}
		, m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]};
}

void save(std::string const filename)
{
	std::vector<PositionOnDisk> v;

	for (unsigned int i = 0; i != sequences.size(); ++i)
		for (auto && pos : sequences[i])
			v.push_back({i, pos});


	std::ofstream f(filename, std::ios::binary);

	std::copy_n(reinterpret_cast<char const *>(v.data()), v.size() * sizeof(PositionOnDisk), std::ostreambuf_iterator<char>(f));
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

Sequence & sequence() { return sequences[current_sequence]; }
Position & position() { return sequence()[current_position]; }

Position halfway(Position const & a, Position const & b)
{
	Position r;
	for (auto j : playerJoints) r[j] = (a[j] + b[j]) / 2;
	return r;
}

void add_position()
{
	if (current_position == sequence().size() - 1)
	{
		auto p = position();
		sequence().push_back(p);
		current_position = sequence().size() - 1;
	}
	else
	{
		auto p = halfway(position(), sequence()[current_position + 1]);
		for(int i = 0; i != 6; ++i)
			spring(position());
		++current_position;
		sequence().insert(sequence().begin() + current_position, p);
	}
}

Position * prev_position()
{
	if (current_position != 0) return &sequence()[current_position - 1];
	if (current_sequence != 0) return &sequences[current_sequence - 1].back();
	return nullptr;
}

Position * next_position()
{
	if (current_position != sequence().size() - 1) return &sequence()[current_position + 1];
	if (current_sequence != sequences.size() - 1) return &sequences[current_sequence + 1].front();
	return nullptr;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_C) // copy
	{
		clipboard = position();
		return;
	}

	if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_V) // paste
	{
		position() = clipboard;
		return;
	}

	if (action == GLFW_PRESS)
		switch (key)
		{
			case GLFW_KEY_INSERT: add_position(); break;

			// set position to prev/next/center

			case GLFW_KEY_Y: if (auto p = prev_position()) position() = *p; break;
			case GLFW_KEY_I: if (auto p = next_position()) position() = *p; break;
			case GLFW_KEY_U:
				if (auto next = next_position())
				if (auto prev = prev_position())
				{
					position() = halfway(*prev, *next);
					for(int i = 0; i != 6; ++i)
						spring(position());
				}
				break;

			// set joint to prev/next/center

			case GLFW_KEY_H: if (auto p = prev_position()) position()[closest_joint] = (*p)[closest_joint]; break;
			case GLFW_KEY_K: if (auto p = next_position()) position()[closest_joint] = (*p)[closest_joint]; break;
			case GLFW_KEY_J:
				if (auto next = next_position())
				if (auto prev = prev_position())
					position()[closest_joint] = ((*prev)[closest_joint] + (*next)[closest_joint]) / 2;
				break;

			// new sequence

			case GLFW_KEY_N:
			{
				auto p = position();
				sequences.push_back(Sequence{p});
				current_sequence = sequences.size() - 1;
				current_position = 0;
				break;
			}

			case GLFW_KEY_S: save("positions.dat"); break;
			case GLFW_KEY_DELETE:
			{
				if (sequence().size() > 1)
				{
					sequence().erase(sequence().begin() + current_position);
					if (current_position == sequence().size()) --current_position;
				}

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

constexpr unsigned candidates_shown = 5; // todo: define keys for increasing/decreasing

void drawJoint(PlayerJoint pj)
{
	bool const highlight = pj == (chosen_joint ? *chosen_joint : closest_joint);

	glColor(highlight ? green : playerDefs[pj.player].color);

	glPushMatrix();
		glTranslate(position()[pj]);
		GLUquadricObj * Sphere = gluNewQuadric();

		gluSphere(Sphere, jointDefs[pj.joint].radius, 20, 20);

		gluDeleteQuadric(Sphere);
	glPopMatrix();
}

void drawJoints()
{
	for (auto pj : playerJoints) drawJoint(pj);
}

template<typename F>
void determineCandidates(F distance_to_cursor)
{
	candidates.clear();

	PlayerJoint const highlight_joint = chosen_joint ? *chosen_joint : closest_joint;

	for (unsigned seq = 0; seq != sequences.size(); ++seq)
	for (unsigned pos = 0; pos != sequences[seq].size(); ++pos)
	{
		if (dist(position(), sequences[seq][pos]) < 15 &&
			distance_to_cursor(sequences[seq][pos][highlight_joint]) <= 0.3)
			candidates.emplace_back(seq, pos);
	}
}

template<typename F>
void determineNearestJoint(F distance_to_cursor)
{
	double closest = 200;

	for (int i = 0; i != 2; ++i)
	{
		auto & player = position().players[i];

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

		for (auto && candidate : candidates)
		{
			double const d = distance_to_cursor(sequences[candidate.first][candidate.second][*chosen_joint]);

			if (d < best)
			{
				best = d;
				current_sequence = candidate.first;
				current_position = candidate.second;
			}
		}
	}
}

// todo: when reading file, warn about sequences whose start/end isn't connected to anything

void prepareDraw(M const & persp, int width, int height)
{
	glViewport(0, 0, width, height);
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
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);

	grid();
	render(position(), red, blue);

	drawJoints();

	glDisable(GL_DEPTH);
	glDisable(GL_DEPTH_TEST);

	drawJoint(chosen_joint ? *chosen_joint : closest_joint);
}

V2 world2xy(V3 v, M const & persp)
{
	auto v4 = V4(v, 1);
	v4 = yrot(camera.x) * v4;
	v4 = xrot(camera.y) * v4;
	v4.z -= camera.z;
	v4 = persp * v4;
	return xy(v4) / v4.w;
}

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

	glfwSetScrollCallback(window, [](GLFWwindow * window, double xoffset, double yoffset)
		{
			if (yoffset == -1)
			{
				if (current_position != 0) --current_position;
			}
			else if (yoffset == 1)
			{
				if (current_position != sequence().size() - 1) ++current_position;
			}
		});
	
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.y -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.y += 0.05;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.x -= 0.03;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.x += 0.03;
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) camera.z -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) camera.z += 0.05;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		float const ratio = width / (float) height;

		auto const persp = perspective(90, ratio, 0.1, 8);

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		V2 const cursor{((xpos / width) - 0.5) * 2, ((1-(ypos / height)) - 0.5) * 2};

		auto distance_to_cursor = [&](V3 v){ return norm2(world2xy(v, persp) - cursor); };

		determineCandidates(distance_to_cursor);

		// editing

		if (chosen_joint && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			Position new_pos = position();

			V4 const dragger = yrot(-camera.x) * V4{{1,0,0},0};

			auto & joint = new_pos[*chosen_joint];

			auto off = world2xy(joint, persp) - cursor;

			joint.x -= dragger.x * off.x;
			joint.z -= dragger.z * off.x;
			joint.y = std::max(0., joint.y - off.y);

			spring(new_pos);

			position() = new_pos;
		}

		explore(distance_to_cursor);

		determineNearestJoint(distance_to_cursor);
		
		prepareDraw(persp, width, height);
		draw();

		// draw sequence lines

		glLineWidth(2);

		PlayerJoint const highlight_joint = chosen_joint ? *chosen_joint : closest_joint;

		auto j = highlight_joint;

		glColor(j == highlight_joint ? green : V3{0.5,0.5,0.5});

		glBegin(GL_LINES);
			for (unsigned seq = 0; seq != sequences.size(); ++seq)
			for (unsigned pos = 0; pos <= std::max(0, int(sequences[seq].size()) - 2); ++pos)
			{
				if (dist(position(), sequences[seq][pos]) > 15 ||
					dist(position(), sequences[seq][pos + 1]) > 15) continue;

				auto a = sequences[seq][pos][j];
				auto b = sequences[seq][pos + 1][j];

				if (distance_to_cursor(a) <= 0.3 && distance_to_cursor(b) <= 0.3)
				{
					glVertex(a);
					glVertex(b);
				}
			}
		glEnd();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}
