/***********************************************************************
VruiXine - A VR video player based on Vrui and the xine multimedia
engine.
Copyright (c) 2015-2016 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "video_monitor.hpp"
#include "xine.h"

typedef GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3> Vertex; // Type for vertices to render the video display screen

namespace GrappleMap {

void VideoMonitor::frame()
	{
	/* Lock the most recent frame in the video frame buffer: */
	if(videoFrames.lockNewValue())
		{
		/* Invalidate the current video frame texture: */
		++videoFrameVersion;
		
		/* Check if the screen format changed: */
		const VideoFrame& f=videoFrames.getLockedValue();
		if(frameSize[0]!=f.size[0]-crop[0]-crop[1]||frameSize[1]!=f.size[1]-crop[2]-crop[3]||aspectRatio!=f.aspectRatio)
			{
			++screenParametersVersion;
			frameSize[0]=f.size[0]-crop[0]-crop[1];
			frameSize[1]=f.size[1]-crop[2]-crop[3];
			aspectRatio=videoFrames.getLockedValue().aspectRatio;
			}
		}
	
	}

void VideoMonitor::display() const
	{
	/* Handle the current video frame based on its format: */
	const VideoFrame& frame=videoFrames.getLockedValue();
	int shaderIndex=0;

	if(frame.format==XINE_VORAW_YV12)
		{
		/* Bind the YV12 shader: */
		shaderIndex=0;
		videoShaders[shaderIndex].useProgram();
		
		/* Bind the Y texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,frameTextureIds[0]);
		glUniform1iARB(videoShaderUniforms[shaderIndex][2],0);
		if(frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0],frameSize[1],0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[0].data());
			}

		int const pitch = ((frame.size[0]+15)/16) * 8;

		/* Bind the U texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,frameTextureIds[1]);
		glUniform1iARB(videoShaderUniforms[shaderIndex][3],1);
		if(frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]>>1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]>>1);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0]>>1,frameSize[1]>>1,0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[1].data());
			}
		
		/* Bind the V texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,frameTextureIds[2]);
		glUniform1iARB(videoShaderUniforms[shaderIndex][4],2);
		if(frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]>>1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]>>1);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0]>>1,frameSize[1]>>1,0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[2].data());
			}
		
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		}
	else if(frame.format==XINE_VORAW_YUY2)
		{
		/* Bind the interleaved YU/Y2 texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,frameTextureIds[0]);
		if(frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RG8,frameSize[0],frameSize[1],0,GL_RG,GL_UNSIGNED_BYTE,frame.planes[0].data());
			}
		}
	else if(frame.format==XINE_VORAW_RGB)
		{
		/* Bind the RGB shader: */
		shaderIndex=2;
		videoShaders[shaderIndex].useProgram();
		
		/* Bind the RGB texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,frameTextureIds[0]);
		glUniform1iARB(videoShaderUniforms[shaderIndex][2],0);
		if(frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[2]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB8,frameSize[0],frameSize[1],0,GL_RGB,GL_UNSIGNED_BYTE,frame.planes[0].data());
			}
		}
	else return; // throw std::runtime_error("unrecognized format");
	
	/* Mark the texture objects as up-to-date: */
	frameTextureVersion=videoFrameVersion;
	
	/* Set up a texture transformation: */
	GLfloat texScale[4]={1.0f,1.0f,0.0f,0.0f};
	GLfloat texOffset[4]={0.0f,0.0f,0.0f,0.0f};
	glUniform4fvARB(videoShaderUniforms[shaderIndex][0],1,texScale);
	glUniform4fvARB(videoShaderUniforms[shaderIndex][1],1,texOffset);
	
	/* Bind the vertex and index buffers to render the video screen: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,vertexBufferId);
	
	/* Check if the vertex data is outdated: */
	if(bufferDataVersion!=screenParametersVersion)
		{
		/* Re-generate the vertex buffer contents: */
		Vertex* vertices=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		Vertex* vPtr=vertices;

		double screenWidth=screenHeight*aspectRatio;

		/* Generate a flat screen: */
		double const
			x0=-Math::div2(screenWidth),
			z0=-Math::div2(screenHeight);
		
		GLfloat const
			tsx=GLfloat(frameSize[0])/GLfloat(numVertices[0]-1),
			tsy=GLfloat(frameSize[1])/GLfloat(numVertices[1]-1);

		double const
			sx=screenWidth/double(numVertices[0]-1),
			sz=screenHeight/double(numVertices[1]-1);

		for(int y=0;y<numVertices[1];++y)
			for(int x=0;x<numVertices[0];++x,++vPtr)
				{
				vPtr->texCoord[0]=GLfloat(x)*tsx;
				vPtr->texCoord[1]=GLfloat(frameSize[1])-GLfloat(y)*tsy;
				
				vPtr->position[0]=GLfloat(0);
				vPtr->position[1]=GLfloat(z0+double(y)*sz);
				vPtr->position[2]=GLfloat(x0+double(x)*sx);
				}

		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		bufferDataVersion=screenParametersVersion;
		}
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexBufferId);
	
	/* Draw the screen: */
	glPushMatrix();
	glRotated(90,0.0,1.0,0.0);
	glTranslated(4 /* todo: one of the slided values */, 0, 0);
	
	GLVertexArrayParts::enable(Vertex::getPartsMask());
	glVertexPointer(static_cast<const Vertex*>(0));
	const GLushort* indexPtr=0;
	for(int y=1;y<numVertices[1];++y,indexPtr+=numVertices[0]*2)
		glDrawElements(GL_QUAD_STRIP,numVertices[0]*2,GL_UNSIGNED_SHORT,indexPtr);
	GLVertexArrayParts::disable(Vertex::getPartsMask());
	
	/* Protect the vertex buffers: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	/* Protect the texture objects: */
	if(frame.format==XINE_VORAW_YV12)
		{
		glActiveTextureARB(GL_TEXTURE0_ARB+2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		glActiveTextureARB(GL_TEXTURE0_ARB+1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		}
	else if(frame.format==XINE_VORAW_YV12||frame.format==XINE_VORAW_RGB)
		{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		}
	
	/* Disable the video shaders: */
	GLShader::disablePrograms();

	glPopMatrix();
	}

void VideoMonitor::setCropLeft(int newCropLeft)
	{
	if(crop[0]==newCropLeft)
		return;
	
	/* Change the left crop: */
	crop[0]=newCropLeft;
	frameSize[0]=videoFrames.getLockedValue().size[0]-crop[0]-crop[1];
	++screenParametersVersion;
	++videoFrameVersion;
	}

void VideoMonitor::setCropRight(int newCropRight)
	{
	if(crop[1]==newCropRight)
		return;
	
	/* Change the right crop: */
	crop[1]=newCropRight;
	frameSize[0]=videoFrames.getLockedValue().size[0]-crop[0]-crop[1];
	++screenParametersVersion;
	++videoFrameVersion;
	}

void VideoMonitor::setCropBottom(int newCropBottom)
	{
	if(crop[2]==newCropBottom)
		return;
	
	/* Change the bottom crop: */
	crop[2]=newCropBottom;
	frameSize[1]=videoFrames.getLockedValue().size[1]-crop[2]-crop[3];
	++screenParametersVersion;
	++videoFrameVersion;
	}

void VideoMonitor::setCropTop(int newCropTop)
	{
	if(crop[3]==newCropTop)
		return;
	
	/* Change the top crop: */
	crop[3]=newCropTop;
	frameSize[1]=videoFrames.getLockedValue().size[1]-crop[2]-crop[3];
	++screenParametersVersion;
	++videoFrameVersion;
	}

VideoMonitor::VideoMonitor()
	{
	/* Initialize all required OpenGL extensions: */
	GLARBMultitexture::initExtension();
	GLARBTextureRectangle::initExtension();
	GLARBTextureRg::initExtension();
	GLARBVertexBufferObject::initExtension();
	
	/* Create the vertex and index buffers: */
	glGenBuffersARB(1,&vertexBufferId);
	glGenBuffersARB(1,&indexBufferId);
	
	/* Create three frame texture objects: */
	glGenTextures(3,frameTextureIds);

	VideoMonitor * dataItem = this; // todo: remove

	dataItem->numVertices[0]=64;
	dataItem->numVertices[1]=32;
	
	/* Create vertex buffer to render the video screen: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,dataItem->numVertices[1]*dataItem->numVertices[0]*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	/* Create an index buffer to render the video screen as a set of quad strips: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferId);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,(dataItem->numVertices[1]-1)*dataItem->numVertices[0]*2*sizeof(GLushort),0,GL_STATIC_DRAW_ARB);
	
	/* Generate indices to render quad strips: */
	GLushort* indices=static_cast<GLushort*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
	GLushort* iPtr=indices;
	for(int y=1;y<dataItem->numVertices[1];++y)
		for(int x=0;x<dataItem->numVertices[0];++x,iPtr+=2)
			{
			iPtr[0]=y*dataItem->numVertices[0]+x;
			iPtr[1]=(y-1)*dataItem->numVertices[0]+x;
			}
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	/* Initialize the three video plane textures: */
	GLenum clampMode=GL_CLAMP_TO_EDGE;
	GLenum sampleMode=GL_LINEAR;
	for(int i=0;i<3;++i)
		{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[i]);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,clampMode);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,clampMode);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,sampleMode);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,sampleMode);
		}
	
	/* Protect all texture objects: */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
	
	/* Create a shader to render video frames in YV12 format: */
	static const char* vertexShaderSourceYV12="\
		uniform vec4 texScale,texOffset; \n\
		 \n\
		void main() \n\
			{ \n\
			/* Copy the texture coordinate: */ \n\
			gl_TexCoord[0]=gl_MultiTexCoord0*texScale+texOffset; \n\
			 \n\
			/* Transform the vertex: */ \n\
			gl_Position=ftransform(); \n\
			} \n\
		";
	
	static const char* fragmentShaderSourceYV12="\
		#extension GL_ARB_texture_rectangle : enable \n\
		 \n\
		uniform sampler2DRect ypTextureSampler; // Sampler for input Y' texture \n\
		uniform sampler2DRect cbTextureSampler; // Sampler for input Cb texture \n\
		uniform sampler2DRect crTextureSampler; // Sampler for input Cr texture \n\
		 \n\
		void main() \n\
			{ \n\
			/* Get the interpolated texture color in Y'CbCr space: */ \n\
			vec3 ypcbcr; \n\
			ypcbcr.r=texture2DRect(ypTextureSampler,gl_TexCoord[0].st).r; \n\
			ypcbcr.g=texture2DRect(cbTextureSampler,gl_TexCoord[0].st*vec2(0.5)).r; \n\
			ypcbcr.b=texture2DRect(crTextureSampler,gl_TexCoord[0].st*vec2(0.5)).r; \n\
			 \n\
			/* Convert the color to RGB directly: */ \n\
			float grey=(ypcbcr.r-16.0/255.0)*1.16438; \n\
			vec4 rgb; \n\
			rgb.r=grey+(ypcbcr.b-128.0/255.0)*1.59603; \n\
			rgb.g=grey-(ypcbcr.g-128.0/255.0)*0.391761-(ypcbcr.b-128.0/255.0)*0.81297; \n\
			rgb.b=grey+(ypcbcr.g-128.0/255.0)*2.01723; \n\
			rgb.a=1.0; \n\
			\n\
			/* Store the final color: */ \n\
			gl_FragColor=rgb; \n\
			} \n\
		";
	
	dataItem->videoShaders[0].compileVertexShaderFromString(vertexShaderSourceYV12);
	dataItem->videoShaders[0].compileFragmentShaderFromString(fragmentShaderSourceYV12);
	dataItem->videoShaders[0].linkShader();
	dataItem->videoShaderUniforms[0][0]=dataItem->videoShaders[0].getUniformLocation("texScale");
	dataItem->videoShaderUniforms[0][1]=dataItem->videoShaders[0].getUniformLocation("texOffset");
	dataItem->videoShaderUniforms[0][2]=dataItem->videoShaders[0].getUniformLocation("ypTextureSampler");
	dataItem->videoShaderUniforms[0][3]=dataItem->videoShaders[0].getUniformLocation("cbTextureSampler");
	dataItem->videoShaderUniforms[0][4]=dataItem->videoShaders[0].getUniformLocation("crTextureSampler");
	
	/* Create a shader to render video frames in RGB format: */
	static const char* vertexShaderSourceRGB="\
		uniform vec4 texScale,texOffset; \n\
		 \n\
		void main() \n\
			{ \n\
			/* Copy the texture coordinate: */ \n\
			gl_TexCoord[0]=gl_MultiTexCoord0*texScale+texOffset; \n\
			 \n\
			/* Transform the vertex: */ \n\
			gl_Position=ftransform(); \n\
			} \n\
		";
	
	static const char* fragmentShaderSourceRGB="\
		#extension GL_ARB_texture_rectangle : enable \n\
		 \n\
		uniform sampler2DRect rgbTextureSampler; // Sampler for input RGB texture \n\
		 \n\
		void main() \n\
			{ \n\
			/* Get the interpolated color in RGB space: */ \n\
			gl_FragColor=texture2DRect(rgbTextureSampler,gl_TexCoord[0].st); \n\
			} \n\
		";
	
	dataItem->videoShaders[2].compileVertexShaderFromString(vertexShaderSourceRGB);
	dataItem->videoShaders[2].compileFragmentShaderFromString(fragmentShaderSourceRGB);
	dataItem->videoShaders[2].linkShader();
	dataItem->videoShaderUniforms[2][0]=dataItem->videoShaders[2].getUniformLocation("texScale");
	dataItem->videoShaderUniforms[2][1]=dataItem->videoShaders[2].getUniformLocation("texOffset");
	dataItem->videoShaderUniforms[2][2]=dataItem->videoShaders[2].getUniformLocation("rgbTextureSampler");
	}

VideoMonitor::~VideoMonitor()
	{
	/* Delete the vertex and index buffers: */
	glDeleteBuffersARB(1,&vertexBufferId);
	glDeleteBuffersARB(1,&indexBufferId);
	
	/* Delete the three frame texture objects: */
	glDeleteTextures(3,frameTextureIds);
	}

}
