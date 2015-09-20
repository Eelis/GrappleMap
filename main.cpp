#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>
#include <GL/glu.h>

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

struct V { GLdouble x, y, z; };

double norm(V v){ return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

V operator-(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V operator+(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V operator*(V v, double s) { return {v.x * s, v.y * s, v.z * s}; }
V & operator+=(V & a, V b) { return a = a + b; }
V & operator-=(V & a, V b) { return a = a - b; }

std::ostream & operator<<(std::ostream & o, V v){ return o << '{' << v.x << ',' << v.y << ',' << v.z << '}'; }

V operator/(V v, double s) { return {v.x / s, v.y / s, v.z / s}; }
V normalize(V v){ return v / norm(v); }

inline void glVertex(V const & v) { glVertex3d(v.x, v.y, v.z); }

struct Player
{
	std::array<V, joint_count> joints;

	Player()
	{
		for (auto && j : joints) j = {(rand()%20)-10, (rand()%20)-10, (rand()%20)-10};
	}
};


double distance(V from, V to)
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
					V dir = normalize(p.joints[s.end] - p.joints[s.start]);
					r.joints[j] -= dir * force;
				}
			}
			else if (s.end == j)
			{
				double force = (s.length - distance(p.joints[s.end], p.joints[s.start])) / 10;
				if (std::abs(force) > 0.001)
				{
					V dir = normalize(p.joints[s.start] - p.joints[s.end]);
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

Position position;
V camera{};

void grid()
{
	glBegin(GL_LINES);
		for (int i = -10; i <= 10; ++i)
		{
			glVertex3f(i, -1, -10);
			glVertex3f(i, -1, 10);
			glVertex3f(-10, -1, i);
			glVertex3f(10, -1, i);
		}
	glEnd();
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
		gluPerspective(60, ratio, 0.1, 30);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(-camera.x, -camera.y, -camera.z);

		glScalef(0.2,0.2,0.2);

		grid();

		render(position);

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			camera.y += 0.01;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			camera.y -= 0.01;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			camera.x -= 0.01;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			camera.x += 0.01;
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS)
			camera.z -= 0.01;
		if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS)
			camera.z += 0.01;

		spring(position);
	}

	glfwTerminate();
}
