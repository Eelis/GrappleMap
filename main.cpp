#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <GL/glu.h>
#include <vector>

enum Joint: uint32_t
{
	LeftToe, RightToe,
	LeftHeel, RightHeel,
	LeftAnkle, RightAnkle,
	LeftKnee, RightKnee,
	LeftHip, RightHip,
	LeftHand, RightHand,
	LeftWrist, RightWrist,
	LeftElbow, RightElbow,
	LeftShoulder, RightShoulder,
	Neck, Head
};

constexpr uint32_t joint_count = 18;

struct Segment
{
	Joint start, end;
	double length; // in meters
};

const Segment segments[] =
  { {LeftToe, LeftHeel, 0.22}
  , {LeftToe, LeftAnkle, 0.18}
  , {LeftHeel, LeftAnkle, 0.1}
  , {LeftAnkle, LeftKnee, 0.42}
  , {LeftKnee, LeftHip, 0.47}
  , {LeftHip, LeftShoulder, 0.5}
  , {LeftShoulder, LeftElbow, 0.29}
  , {LeftElbow, LeftWrist, 0.26}
  , {LeftWrist, LeftHand, 0.17}

  , {RightToe, RightHeel, 0.22}
  , {RightToe, RightAnkle, 0.18}
  , {RightHeel, RightAnkle, 0.1}
  , {RightAnkle, RightKnee, 0.42}
  , {RightKnee, RightHip, 0.47}
  , {RightHip, RightShoulder, 0.5}
  , {RightShoulder, RightElbow, 0.29}
  , {RightElbow, RightWrist, 0.26}
  , {RightWrist, RightHand, 0.17}

  , {LeftShoulder, RightShoulder, 0.4}
  , {LeftHip, RightHip, 0.32}
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
				double force = (s.length - distance(p.joints[s.end], p.joints[s.start])) / 10;
				if (std::abs(force) > 0.001)
				{
					V3 dir = normalize(p.joints[s.end] - p.joints[s.start]);
					r.joints[j] -= dir * force;
				}
			}
			else if (s.end == j)
			{
				double force = (s.length - distance(p.joints[s.end], p.joints[s.start])) / 10;
				if (std::abs(force) > 0.001)
				{
					V3 dir = normalize(p.joints[s.start] - p.joints[s.end]);
					r.joints[j] -= dir * force;
				}
			}
		}
	}

	return r;
}

struct Position
{
	Player players[2];
};

void spring(Position & pos)
{
	for (auto && p : pos.players) p = spring(p);
}

void render(Player const & p)
{
	for (auto && s : segments)
	{
		glVertex(p.joints[s.start]);
		glVertex(p.joints[s.end]);
	}
}

void render(Position const & p)
{
	glBegin(GL_LINES);
		render(p.players[0]);
		render(p.players[1]);
	glEnd();
}

std::vector<Position> positions(1);
unsigned current_pos = 0;

V3 camera{0,0,5};
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
	glBegin(GL_LINES);
		for (int i = -4; i <= 4; ++i)
		{
			glVertex3f(i, -1, -4);
			glVertex3f(i, -1, 4);
			glVertex3f(-4, -1, i);
			glVertex3f(4, -1, i);
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

		glPointSize(10);

		auto & position = positions[current_pos];

		glColor3f(1,1,1);
		grid();
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
			rotation -= 0.01;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			rotation += 0.01;
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS)
			camera.z -= 0.05;
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS)
			camera.z += 0.05;

		spring(position);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			closest_player->joints[closest_ji].x -= dragger.x * closest_off.x;
			closest_player->joints[closest_ji].z -= dragger.z * closest_off.x;

			closest_player->joints[closest_ji].y -= closest_off.y;
		}

	}

	glfwTerminate();
}
