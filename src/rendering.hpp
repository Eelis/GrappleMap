#ifndef JIUJITSUMAPPER_RENDERING_HPP
#define JIUJITSUMAPPER_RENDERING_HPP

#include <GL/glu.h>
#include "math.hpp"
#include "viables.hpp"

inline void glVertex(V3 const & v) { glVertex3d(v.x, v.y, v.z); }
inline void glNormal(V3 const & v) { glNormal3d(v.x, v.y, v.z); }
inline void glTranslate(V3 const & v) { glTranslated(v.x, v.y, v.z); }
inline void glColor(V3 v) { glColor3d(v.x, v.y, v.z); }

void grid();

void render(Viables const *, Position const &,
	boost::optional<PlayerJoint> highlight_joint, bool edit_mode);

struct Camera;
struct Graph;

void drawViables(Graph const &, Viables const &, PlayerJoint);

void prepareDraw(Camera const &, int width, int height);

#endif
