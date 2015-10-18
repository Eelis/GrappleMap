#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

#include <GL/glu.h>
#include "math.hpp"
#include "viables.hpp"

inline void glVertex(V3 const & v) { glVertex3d(v.x, v.y, v.z); }
inline void glNormal(V3 const & v) { glNormal3d(v.x, v.y, v.z); }
inline void glTranslate(V3 const & v) { glTranslated(v.x, v.y, v.z); }
inline void glColor(V3 v) { glColor3d(v.x, v.y, v.z); }

inline void gluLookAt(V3 eye, V3 center, V3 up)
{
	gluLookAt(
		eye.x, eye.y, eye.z,
		center.x, center.y, center.z,
		up.x, up.y, up.z);
}

void grid();

void render(Viables const *, Position const &,
	boost::optional<PlayerJoint> highlight_joint,
	boost::optional<unsigned> first_person_player,
	bool edit_mode);

struct Camera;
struct Graph;

void drawViables(Graph const &, Viables const &, PlayerJoint);

void prepareDraw(Camera &, int x, int y, int width, int height);

#endif
