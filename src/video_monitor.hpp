#ifndef GRAPPLEMAP_VIDEO_MONITOR_HPP
#define GRAPPLEMAP_VIDEO_MONITOR_HPP

#include <Threads/TripleBuffer.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBTextureRectangle.h>
#include <GL/Extensions/GLARBTextureRg.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLShader.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLGeometryVertex.h>

namespace GrappleMap
{
	struct VideoFrame
	{
		int format = 0;
		int size[2] = {0,0};
		double aspectRatio = 0;
		std::vector<unsigned char> planes[3]; // Up to three video frame planes
	};

	class VideoMonitor
	{
		int numVertices[2]; // Number of vertices to render the video screen
		GLuint vertexBufferId=0; // Buffer for vertices to render the video screen
		GLuint indexBufferId = 0; // Buffer for vertex indices to render the video screen
		mutable unsigned int bufferDataVersion = 0; // Version number of vertex data in vertex buffer
		GLuint frameTextureIds[3]; // IDs of three frame textures (Y plane or interleaved YUY2 or RGB, U plane, V plane)
		GLShader videoShaders[3]; // Shaders to render video textures in YV12, YUY2, and RGB formats, respectively
		int videoShaderUniforms[3][5]; // Uniform variable locations of the three video rendering shaders
		mutable unsigned int frameTextureVersion=0; // Version number of frame in frame texture(s)

		unsigned int videoFrameVersion = 0; // Version number of currently locked frame
		double screenHeight = 3; // Height of video display screen in navigational coordinate units
		double aspectRatio = 16.0/9.0;  // The aspect ratio of the currently locked video frame
		int crop[4]{};
		int frameSize[2]{}; // The frame size of the currently locked video frame
		unsigned int screenParametersVersion = 1; // Version number of screen parameters, including aspect ratio of current frame

		void setCropLeft(int newCropLeft);
		void setCropRight(int newCropRight);
		void setCropBottom(int newCropBottom);
		void setCropTop(int newCropTop);
		
	public:

		Threads::TripleBuffer<VideoFrame> videoFrames;
		
		VideoMonitor();
		~VideoMonitor();

		void frame();
		void display() const;
	};
}

#endif
