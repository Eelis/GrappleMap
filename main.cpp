#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <GL/glu.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <algorithm>

enum Joint: uint32_t
{
	LeftToe, RightToe,
	LeftHeel, RightHeel,
	LeftAnkle, RightAnkle,
	LeftKnee, RightKnee,
	LeftHip, RightHip,
	LeftShoulder, RightShoulder,
	LeftElbow, RightElbow,
	LeftWrist, RightWrist,
	LeftHand, RightHand,
	LeftFingers, RightFingers,
	Core, Neck, Head
};

constexpr uint32_t joint_count = 24;

struct Segment
{
	Joint start, end;
	double length; // in meters
	bool visible;
};

const Segment segments[] =
  { {LeftToe, LeftHeel, 0.22, false}
  , {LeftToe, LeftAnkle, 0.18, true}
  , {LeftHeel, LeftAnkle, 0.07, true}
  , {LeftAnkle, LeftKnee, 0.42, true}
  , {LeftKnee, LeftHip, 0.45, true}
  , {LeftHip, Core, 0.3, true}
  , {Core, LeftShoulder, 0.47, true}
  , {LeftShoulder, LeftElbow, 0.29, true}
  , {LeftElbow, LeftWrist, 0.26, true}
  , {LeftWrist, LeftHand, 0.08, true}
  , {LeftHand, LeftFingers, 0.08, true}
  , {LeftWrist, LeftFingers, 0.14, false}

  , {RightToe, RightHeel, 0.22, false}
  , {RightToe, RightAnkle, 0.18, true}
  , {RightHeel, RightAnkle, 0.07, true}
  , {RightAnkle, RightKnee, 0.42, true}
  , {RightKnee, RightHip, 0.45, true}
  , {RightHip, Core, 0.3, true}
  , {Core, RightShoulder, 0.47, true}
  , {RightShoulder, RightElbow, 0.29, true}
  , {RightElbow, RightWrist, 0.26, true}
  , {RightWrist, RightHand, 0.08, true}
  , {RightHand, RightFingers, 0.08, true}
  , {RightWrist, RightFingers, 0.14, false}

  , {LeftShoulder, RightShoulder, 0.4, true}
  , {LeftHip, RightHip, 0.30, true}

  , {LeftShoulder, Neck, 0.25, true}
  , {RightShoulder, Neck, 0.25, true}
  , {Neck, Head, 0.25, true}
  };

struct V2 { GLdouble x, y; };

struct V3 { GLdouble x, y, z; };

struct V4
{
	GLdouble x, y, z, w;

	V4(V3 v, GLdouble w)
	  : x(v.x), y(v.y), z(v.z), w(w)
	{}
};

V2 xy(V3 v){ return {v.x, v.y}; }
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
V4 operator-(V4 a, V4 b) { return {{a.x - b.x, a.y - b.y, a.z - b.z}, a.w - b.w}; }
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V4 operator+(V4 a, V4 b) { return {{a.x + b.x, a.y + b.y, a.z + b.z}, a.w + b.w}; }
V3 operator*(V3 v, double s) { return {v.x * s, v.y * s, v.z * s}; }
V3 & operator+=(V3 & a, V3 b) { return a = a + b; }
V3 & operator-=(V3 & a, V3 b) { return a = a - b; }

std::ostream & operator<<(std::ostream & o, V3 v){ return o << '{' << v.x << ',' << v.y << ',' << v.z << '}'; }

V3 operator/(V3 v, double s) { return {v.x / s, v.y / s, v.z / s}; }
V3 normalize(V3 v){ return v / norm(v); }

inline void glVertex(V3 const & v) { glVertex3d(v.x, v.y, v.z); }

struct Player
{
	std::array<V3, joint_count> joints;

	Player()
	{
		for (auto && j : joints) j = {(rand()%6)-3, (rand()%6), (rand()%6)-3};
	}
};


double distance(V3 from, V3 to)
{
	return norm(to - from);
}

Player spring(Player const & p)
{
	Player r;

	for (uint32_t j = 0; j != joint_count; ++j)
	{
		r.joints[j] = p.joints[j];

		for (auto && s : segments)
		{
			if (s.start == j)
			{
				double force = (s.length - distance(p.joints[s.end], p.joints[s.start])) / 5;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.end] - p.joints[s.start]);
					r.joints[j] -= dir * force;
				}
			}
			else if (s.end == j)
			{
				double force = (s.length - distance(p.joints[s.end], p.joints[s.start])) / 5;
				if (std::abs(force) > 0.0001)
				{
					V3 dir = normalize(p.joints[s.start] - p.joints[s.end]);
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

void render(Position const & pos)
{
	glLineWidth(30);
	glBegin(GL_LINES);
		for (auto && p : pos.players)
			for (auto && s : segments)
				if (s.visible)
				{
					glVertex(p.joints[s.start]);
					glVertex(p.joints[s.end]);
				}
	glEnd();

	glPointSize(10);
	glBegin(GL_POINTS);
		for (auto && p : pos.players)
			for (auto && s : segments)
				if (s.visible)
				{
					glVertex(p.joints[s.start]);
					glVertex(p.joints[s.end]);
				}
	glEnd();
}

std::vector<Position> positions = load("positions.dat");
Position position = positions.front();
unsigned current_pos = 0;

V3 camera{0,1.7,2};
double rotation = 0;

V4 operator*(M const & m, V4 v)
{
	return
		{{ m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]
		, m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]
		, m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]}
		, m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]};
}

void grid()
{
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		switch (key)
		{
			case GLFW_KEY_S:
			{
				positions.push_back(position);
				save("positions.dat", positions);
				break;
			}
			case GLFW_KEY_N:
			{
				++current_pos %= positions.size();
				position = positions[current_pos];
				break;
			}
		}
}

int main()
{
	if (!glfwInit())
		return -1;

	GLFWwindow * window = glfwCreateWindow(640, 480, "Jiu Jitsu Mapper", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		float ratio = width / (float) height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		auto persp = perspective(90, ratio, 0.1, 30);
		glMultMatrixd(persp.data());

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(-camera.x, -camera.y, -camera.z);

		glMultMatrixd(yrot(rotation).data());

		glColor3f(0.5,0.5,0.5);
		grid();
		for (auto && p : positions) render(p);

		glColor3f(1,1,1);
		render(position);

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		V2 cursor {((xpos / width) - 0.5) * 2, ((1-(ypos / height)) - 0.5) * 2};

		double closest = 100;
		V3 closest_joint;
		V2 closest_off;
		Player * closest_player = &position.players[0];
		unsigned closest_ji = 0;

		V4 dragger{{1,0,0},0};
		dragger = yrot(-rotation) * dragger;

		for (auto && player : position.players)
		{
			for (unsigned ji = 0; ji != joint_count; ++ji)
			{
				V3 j = player.joints[ji];

				auto clipSpace = persp * (yrot(rotation) * V4(j, 1) - V4(camera, 0));

				V3 ndcSpacePos = xyz(clipSpace) / clipSpace.w;

				V2 off = xy(ndcSpacePos) - cursor;

				double d = norm2(off);

				if (d < closest)
				{
					closest = d;
					closest_joint = j;
					closest_off = off;
					closest_ji = ji;
					closest_player = &player;
				}
			}
		}

		glColor3f(1,0,0);
		glPointSize(20);
		glBegin(GL_POINTS);
		glVertex(closest_joint);
		glEnd();

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			camera.y += 0.05;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			camera.y -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			rotation -= 0.02;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			rotation += 0.02;
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS)
			camera.z -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS)
			camera.z += 0.05;

		spring(position);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			closest_player->joints[closest_ji].x -= dragger.x * closest_off.x;
			closest_player->joints[closest_ji].z -= dragger.z * closest_off.x;

			auto & y = closest_player->joints[closest_ji].y;
			y = std::max(0., y - closest_off.y);
		}

	}

	glfwTerminate();
}
