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

#ifndef VRUIXINE_HPP
#define VRUIXINE_HPP

#include <string.h>
#include <xine.h>
#include <xine/xineutils.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Plane.h>
#include <Geometry/LinearUnit.h>
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
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Glyph.h>
#include <GLMotif/Button.h>
#include <GLMotif/NewButton.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextField.h>
#include <GLMotif/Slider.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Viewer.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/GenericAbstractToolFactory.h>
#include <Vrui/DisplayState.h>
#include <Vrui/OpenFile.h>

class VruiXine
	{
	/* Embedded classes: */
	private:
	struct Frame // Structure representing a video frame
		{
		/* Elements: */
		public:
		int format; // Format of video frame (YV12, YUY2, or RGB)
		int size[2]; // Size of video frame
		Vrui::Scalar aspectRatio; // Display aspect ratio of video frame
		unsigned char* planes[3]; // Pointers to up to three video frame planes
		
		/* Constructors and destructors: */
		Frame(void) // Creates an empty frame
			:format(0),
			 aspectRatio(0)
			{
			size[0]=size[1]=0;
			planes[0]=planes[1]=planes[2]=0;
			}
		~Frame(void) // Destroys a frame
			{
			for(int i=0;i<3;++i)
				delete[] planes[i];
			}
		};
	
	struct OverlaySet // Structure representing a set of overlays
		{
		/* Elements: */
		public:
		int numOverlays; // Number of active overlays
		raw_overlay_t overlays[XINE_VORAW_MAX_OVL];
		
		/* Constructors and destructors: */
		OverlaySet(void) // Creates an empty overlay set
			:numOverlays(0)
			{
			for(int i=0;i<XINE_VORAW_MAX_OVL;++i)
				overlays[i].ovl_rgba=0;
			}
		~OverlaySet(void) // Destroys an overlay set
			{
			for(int i=0;i<XINE_VORAW_MAX_OVL;++i)
				delete[] overlays[i].ovl_rgba;
			}
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		int numVertices[2]; // Number of vertices to render the video screen
		GLuint vertexBufferId; // Buffer for vertices to render the video screen
		GLuint indexBufferId; // Buffer for vertex indices to render the video screen
		unsigned int bufferDataVersion; // Version number of vertex data in vertex buffer
		GLuint frameTextureIds[3]; // IDs of three frame textures (Y plane or interleaved YUY2 or RGB, U plane, V plane)
		GLShader videoShaders[3]; // Shaders to render video textures in YV12, YUY2, and RGB formats, respectively
		int videoShaderUniforms[3][5]; // Uniform variable locations of the three video rendering shaders
		unsigned int frameTextureVersion; // Version number of frame in frame texture(s)
		GLuint overlayTextureIds[XINE_VORAW_MAX_OVL]; // IDs of textures holding overlays
		unsigned int overlayTextureVersion; // Version number of overlay set in overlay textures
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	typedef GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3> Vertex; // Type for vertices to render the video display screen
	
	/* Elements: */
	xine_t* xine; // Handle to the main xine engine object
	xine_video_port_t* videoOutPort; // Handle to the video output port
	xine_audio_port_t* audioOutPort; // Handle to the audio output port
	xine_stream_t* stream; // Handle to the played multimedia stream
	xine_event_queue_t* eventQueue; // Handle to the event queue
	std::string videoFileName; // File name of the currently playing video
	GLMotif::FileSelectionHelper videoFileSelectionHelper; // Helper object to open video files
	Threads::TripleBuffer<Frame> videoFrames; // Triple buffer of video frames received from the video output plug-in
	unsigned int videoFrameVersion; // Version number of currently locked frame
	Threads::TripleBuffer<OverlaySet> overlaySets; // Triple buffer of overlay sets
	unsigned int overlaySetVersion; // Version number of currently locked overlay set
	GLMotif::PopupWindow* streamControlDialog; // Dialog window to control properties of the played video stream
	GLMotif::RadioBox* stereoModes; // Radio box to select stereo modes
	GLMotif::RadioBox* stereoLayouts; // Radio box to select stereo sub-frame layouts
	GLMotif::ToggleButton* stereoSquashedToggle; // Toggle to select squashed stereo
	GLMotif::ToggleButton* forceMonoToggle; // Toggle to force mono mode on stereo videos
	GLMotif::TextFieldSlider* stereoSeparationSlider; // Slider to adjust stereo separation in stereo videos
	GLMotif::TextFieldSlider* cropSliders[4]; // Sliders to adjust video cropping
	GLMotif::PopupWindow* dvdNavigationDialog; // Dialog window for DVD menu and chapter navigation
	GLMotif::Slider* volumeSlider; // Slider to adjust audio volume
	GLMotif::PopupWindow* playbackControlDialog; // Dialog window to control playback position
	GLMotif::TextField* playbackPositionText; // Text field showing the current playback position
	GLMotif::Slider* playbackSlider; // Slider to drag the current playback position
	bool playbackSliderDragging; // Flag whether the playback slider is currently being dragged
	GLMotif::TextField* streamLengthText; // Text field showing the length of the current stream
	double playbackPosCheckTime; // Next application time at which to check the playback position
	double streamLength; // Most recently reported stream length in seconds
	Vrui::Point screenCenter; // Center of video display screen in navigational coordinates
	Vrui::Scalar screenHeight; // Height of video display screen in navigational coordinate units
	Vrui::Scalar aspectRatio; // The aspect ratio of the currently locked video frame
	double screenAzimuth,screenElevation; // Angles to rotate the screen
	int stereoMode; // Stream's stereo mode, 0: mono, 1: side-by-side, 2: top/bottom
	int stereoLayout; // Layout of sub-frames. 0: Left eye is left or top, 1: Left eye is right or bottom
	bool stereoSquashed; // Flag whether stereo sub-frames are squashed to fit into the original frame
	bool forceMono; // Flag to only use the left eye of stereo pairs for display
	float stereoSeparation; // Extra amount of stereo separation to adjust "depth"
	int crop[4]; // Amount of cropping (left, right, bottom, top) that needs to be applied to incoming video frames
	int frameSize[2]; // The frame size of the currently locked video frame
	unsigned int screenParametersVersion; // Version number of screen parameters, including aspect ratio of current frame
	GLMotif::PopupWindow* screenControlDialog; // Dialog window to control the position and size of the virtual video projection screen
	
	/* Private methods: */
	static void xineEventCallback(void* userData,const xine_event_t* event); // Callback called when a playback event occurs
	static void xineOutputCallback(void* userData,int frameFormat,int frameWidth,int frameHeight,double frameAspect,void* data0,void* data1,void* data2); // Callback called from video output plug-in when a new video frame is ready for display
	static void xineOverlayCallback(void* userData,int numOverlays,raw_overlay_t* overlays); // Callback called from video output plug-in when an overlay needs to be merged into the video stream
	void xineSendEvent(int eventId); // Sends an event to the xine library
	void shutdownXine(void); // Cleanly shuts down the xine library
	void setStereoMode(int newStereoMode);
	void setStereoLayout(int newStereoLayout);
	void setStereoSquashed(bool newStereoSquashed);
	void setForceMono(bool newForceMono);
	void setStereoSeparation(float newStereoSeparation);
	void setCropLeft(int newCropLeft);
	void setCropRight(int newCropRight);
	void setCropBottom(int newCropBottom);
	void setCropTop(int newCropTop);
	void setScreenMode(int newScreenMode);
	void loadVideo(const char* newVideoFileName); // Loads a video stream from the file of the given name
	void stereoModesValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData); // Callback called when the stereo mode is changed
	void stereoLayoutsValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData); // Callback called when the stereo layout is changed
	void stereoSquashedToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData); // Callback called when the stereo squashed button is toggled
	void forceMonoToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData); // Callback called when the force mono button is toggled
	void stereoSeparationSliderValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the stereo separation slider changes value
	void cropSliderValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData,const int& sliderIndex); // Callback called when one of the crop sliders changes value
	GLMotif::PopupWindow* createStreamControlDialog(void); // Creates the stream control dialog
	void dvdNavigationButtonCallback(Misc::CallbackData* cbData,const int& eventId); // Callback called when one of the DVD navigation buttons is pressed
	void loadVideoFileCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback to load a new video file
	void saveConfigurationCallback(Misc::CallbackData* cbData); // Callback to save the configuration for the current video file
	void volumeSliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData); // Callback called when the volume slider changes value
	GLMotif::PopupWindow* createDvdNavigationDialog(void); // Creates the DVD navigation dialog
	void skipBackCallback(Misc::CallbackData* cbData); // Callback called when the "skip back" button is pressed
	void skipAheadCallback(Misc::CallbackData* cbData); // Callback called when the "skip ahead" button is pressed
	void playbackSliderDraggingCallback(GLMotif::DragWidget::DraggingCallbackData* cbData); // Callback called when the playback slider is being dragged
	void playbackSliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData); // Callback called when the playback slider changes value
	GLMotif::PopupWindow* createPlaybackControlDialog(void); // Creates the playback control dialog
	void screenModesValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData); // Callback called when the theater mode toggle is toggled
	void screenDistanceValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the screen distance slider changes value
	void screenHeightValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the screen height slider changes value
	void screenBottomValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the screen bottom slider changes value
	void screenAzimuthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the screen azimuth angle slider changes value
	void screenElevationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData); // Callback called when the screen elevation angle slider changes value
	
	GLMotif::PopupWindow* createScreenControlDialog(void); // Creates the screen control dialog
	
	/* Constructors and destructors: */
	public:
	VruiXine(std::vector<std::string> const & argv);
	virtual ~VruiXine(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData, GLObject const *) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData, GLObject const *) const;
	};

#endif
