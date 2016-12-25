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

#include "VruiXine.hpp"

/***********************************
Methods of class VruiXine::DataItem:
***********************************/

VruiXine::DataItem::DataItem(void)
	:vertexBufferId(0),indexBufferId(0),bufferDataVersion(0),
	 frameTextureVersion(0),
	 overlayTextureVersion(0)
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
	
	/* Create overlay texture objects: */
	glGenTextures(XINE_VORAW_MAX_OVL,overlayTextureIds);
	}

VruiXine::DataItem::~DataItem(void)
	{
	/* Delete the vertex and index buffers: */
	glDeleteBuffersARB(1,&vertexBufferId);
	glDeleteBuffersARB(1,&indexBufferId);
	
	/* Delete the three frame texture objects: */
	glDeleteTextures(3,frameTextureIds);
	
	/* Delete the overlay texture objects: */
	glDeleteTextures(XINE_VORAW_MAX_OVL,overlayTextureIds);
	}

/*************************
Methods of class VruiXine:
*************************/

void VruiXine::xineEventCallback(void* userData,const xine_event_t* event)
	{
	VruiXine* thisPtr=static_cast<VruiXine*>(userData);
	
	switch(event->type)
		{
		case XINE_EVENT_UI_CHANNELS_CHANGED:
			{
			std::cout<<"Xine event: New channel info available"<<std::endl;
			
			/* Query stream position and length: */
			int streamPos,streamTime,streamLength;
			if(xine_get_pos_length(event->stream,&streamPos,&streamTime,&streamLength))
				std::cout<<"\tStream position: "<<streamTime<<" ms out of "<<streamLength<<" ms ("<<double(streamPos)*100.0/65535.0<<"%"<<std::endl;
			
			/* Query stream video parameters: */
			std::cout<<"\tBitrate: "<<xine_get_stream_info(event->stream,XINE_STREAM_INFO_BITRATE)<<std::endl;
			int videoWidth=xine_get_stream_info(event->stream,XINE_STREAM_INFO_VIDEO_WIDTH);
			int videoHeight=xine_get_stream_info(event->stream,XINE_STREAM_INFO_VIDEO_HEIGHT);
			std::cout<<"\tVideo size: "<<videoWidth<<" x "<<videoHeight;
			std::cout<<", aspect ratio "<<double(xine_get_stream_info(event->stream,XINE_STREAM_INFO_VIDEO_RATIO))/10000.0<<std::endl;
			
			if(thisPtr->streamControlDialog!=0)
				{
				/* Update the stream control dialog: */
				for(int i=0;i<4;++i)
					thisPtr->cropSliders[i]->setValueRange(0.0,double(i>=2?videoHeight/4:videoWidth/4),1.0);
				}
			
			/* Invalidate the playback position: */
			thisPtr->playbackPosCheckTime=0.0;
			break;
			}
		
		case XINE_EVENT_UI_SET_TITLE:
			{
			const xine_ui_data_t* uiData=static_cast<const xine_ui_data_t*>(event->data);
			
			std::cout<<"Xine event: Set UI title to "<<uiData->str<<std::endl;
			
			break;
			}
		
		case XINE_EVENT_FRAME_FORMAT_CHANGE:
			{
			const xine_format_change_data_t* formatChangeData=static_cast<xine_format_change_data_t*>(event->data);
			
			std::cout<<"Xine event: Frame format change to size ";
			std::cout<<formatChangeData->width<<" x "<<formatChangeData->height;
			std::cout<<", aspect ratio ";
			switch(formatChangeData->aspect)
				{
				case 1:
					std::cout<<"1:1";
					break;
				
				case 2:
					std::cout<<"4:3";
					break;
				
				case 3:
					std::cout<<"16:9";
					break;
				
				case 4:
					std::cout<<"2.21:1";
					break;
				
				default:
					std::cout<<"[unknown]";
				}
			if(formatChangeData->pan_scan)
				std::cout<<" (pan & scan)";
			std::cout<<std::endl;
			
			break;
			}
		
		case XINE_EVENT_AUDIO_LEVEL:
			{
			const xine_audio_level_data_t* audioLevelData=static_cast<xine_audio_level_data_t*>(event->data);
			
			std::cout<<"Xine event: Audio levels: left ";
			std::cout<<audioLevelData->left<<"%, right "<<audioLevelData->right<<"%";
			if(audioLevelData->mute)
				std::cout<<" (muted)";
			std::cout<<std::endl;
			
			break;
			}
		
		case XINE_EVENT_QUIT:
			std::cout<<"Xine event: Quit"<<std::endl;
			break;
		
		case XINE_EVENT_DROPPED_FRAMES:
			{
			const xine_dropped_frames_t* droppedFramesData=static_cast<xine_dropped_frames_t*>(event->data);
			
			std::cout<<"Xine event: The number of dropped frames IS TOO DAMN HIGH! (";
			std::cout<<(droppedFramesData->skipped_frames+5)/10<<"% skipped, ";
			std::cout<<(droppedFramesData->discarded_frames+5)/10<<"% discarded)"<<std::endl;
			
			break;
			}
		
		default:
			std::cout<<"Xine event: "<<event->type<<std::endl;
		}
	}

void VruiXine::xineOutputCallback(void* userData,int frameFormat,int frameWidth,int frameHeight,double frameAspect,void* data0,void* data1,void* data2)
	{
	VruiXine* thisPtr=static_cast<VruiXine*>(userData);
	
	/* Prepare a new slot in the video frame triple buffer: */
	Frame& newFrame=thisPtr->videoFrames.startNewValue();
	
	/* Re-allocate the frame's storage if the video format has changed: */
	if(newFrame.format!=frameFormat||newFrame.size[0]!=frameWidth||newFrame.size[1]!=frameHeight)
		{
		#if 0
		
		std::cout<<"Video format changed from ";
		switch(newFrame.format)
			{
			case XINE_VORAW_YV12:
				std::cout<<"YpCbCr 4:2:0";
				break;
			
			case XINE_VORAW_YUY2:
				std::cout<<"YpCbCr 4:2:2";
				break;
			
			case XINE_VORAW_RGB:
				std::cout<<"RGB";
				break;
			}
		std::cout<<", "<<newFrame.size[0]<<" x "<<newFrame.size[1]<<" to ";
		switch(frameFormat)
			{
			case XINE_VORAW_YV12:
				std::cout<<"YpCbCr 4:2:0";
				break;
			
			case XINE_VORAW_YUY2:
				std::cout<<"YpCbCr 4:2:2";
				break;
			
			case XINE_VORAW_RGB:
				std::cout<<"RGB";
				break;
			}
		std::cout<<", "<<frameWidth<<" x "<<frameHeight<<std::endl;
		
		#endif
		
		/* Update the frame format: */
		newFrame.format=frameFormat;
		newFrame.size[0]=frameWidth;
		newFrame.size[1]=frameHeight;
		
		/* Allocate new frame storage: */
		if(newFrame.format==XINE_VORAW_YV12)
			{
			/* Allocate full-size Y plane and half-size U and V planes: */
			newFrame.planes[0].resize(newFrame.size[1]*newFrame.size[0]);
			newFrame.planes[1].resize((newFrame.size[1]>>1)*(newFrame.size[0]>>1));
			newFrame.planes[2].resize((newFrame.size[1]>>1)*(newFrame.size[0]>>1));
			}
		else if(newFrame.format==XINE_VORAW_YUY2)
			{
			/* Allocate interleaved YU and Y2 plane: */
			newFrame.planes[0].resize(newFrame.size[1]*newFrame.size[0]*2);
			newFrame.planes[1] = {};
			newFrame.planes[2] = {};
			}
		else if(newFrame.format==XINE_VORAW_RGB)
			{
			/* Allocate interleaved RGB plane: */
			newFrame.planes[0].resize(newFrame.size[1]*newFrame.size[0]*3);
			newFrame.planes[1] = {};
			newFrame.planes[2] = {};
			}
		}
	
	/* Copy the received frame data: */
	if(newFrame.format==XINE_VORAW_YV12)
		{
		memcpy(newFrame.planes[0].data(),data0,newFrame.size[1]*newFrame.size[0]);
		memcpy(newFrame.planes[1].data(),data1,(newFrame.size[1]>>1)*(newFrame.size[0]>>1));
		memcpy(newFrame.planes[2].data(),data2,(newFrame.size[1]>>1)*(newFrame.size[0]>>1));
		}
	else if(newFrame.format==XINE_VORAW_YUY2)
		memcpy(newFrame.planes[0].data(),data0,newFrame.size[1]*newFrame.size[0]*2);
	else if(newFrame.format==XINE_VORAW_RGB)
		memcpy(newFrame.planes[0].data(),data0,newFrame.size[1]*newFrame.size[0]*3);
	
	/* Update the display aspect ratio: */
	newFrame.aspectRatio=Vrui::Scalar(frameAspect);

	/* Record the frame: */
	thisPtr->recordedFrames[thisPtr->oldestRecord] = newFrame;
	if (++thisPtr->oldestRecord == thisPtr->recordedFrames.size())
		thisPtr->oldestRecord = 0;

	/* Post the new video frame and wake up the main thread: */
	thisPtr->videoFrames.postNewValue();
	Vrui::requestUpdate();
	thisPtr->showingRecordedFrame = boost::none;
	}

void VruiXine::gotoRecorded(size_t const framesSinceOldest)
{
	if (showingRecordedFrame == framesSinceOldest) return;

	if (framesSinceOldest >= recordedFrames.size()) return;

	videoFrames.startNewValue() = recordedFrames[
		(oldestRecord + framesSinceOldest + recordedFrames.size()) % recordedFrames.size()];

	microPlaybackSlider->setValue(framesSinceOldest);

	videoFrames.postNewValue();
	Vrui::requestUpdate();
	showingRecordedFrame = framesSinceOldest;
}

double const fps = 30; // todo: get real value

void VruiXine::setTimeRef(boost::optional<double> const t)
{
	if (t)
	{
		if (!showingRecordedFrame) return;

		timeRef = *t - *showingRecordedFrame / fps;
	}
	else timeRef = boost::none;
}

void VruiXine::seek(double const t)
{
	if (timeRef) gotoRecorded((t - *timeRef) * fps);
}

void VruiXine::xineOverlayCallback(void* userData,int numOverlays,raw_overlay_t* overlays)
	{
	VruiXine* thisPtr=static_cast<VruiXine*>(userData);
	
	/* Prepare a new slot in the overlay set triple buffer: */
	OverlaySet& newOverlaySet=thisPtr->overlaySets.startNewValue();
	
	/* Copy the received overlay set: */
	newOverlaySet.numOverlays=numOverlays;
	for(int i=0;i<numOverlays;++i)
		{
		raw_overlay_t& ovl=newOverlaySet.overlays[i];
		delete[] ovl.ovl_rgba;
		ovl.ovl_w=overlays[i].ovl_w;
		ovl.ovl_h=overlays[i].ovl_h;
		ovl.ovl_x=overlays[i].ovl_x;
		ovl.ovl_y=overlays[i].ovl_y;
		ovl.ovl_rgba=new uint8_t[ovl.ovl_h*ovl.ovl_w*4];
		memcpy(ovl.ovl_rgba,overlays[i].ovl_rgba,ovl.ovl_h*ovl.ovl_w*4);
		}
	for(int i=numOverlays;i<XINE_VORAW_MAX_OVL;++i)
		{
		delete[] newOverlaySet.overlays[i].ovl_rgba;
		newOverlaySet.overlays[i].ovl_rgba=0;
		}
	
	/* Post the new overlay set and wake up the main thread: */
	thisPtr->overlaySets.postNewValue();
	Vrui::requestUpdate();
	}

void VruiXine::xineSendEvent(int eventId)
	{
	/* A table to translate from application event indices to xine event types: */
	static const int xineEventTypes[]={
		XINE_EVENT_INPUT_MENU1,XINE_EVENT_INPUT_MENU2,XINE_EVENT_INPUT_MENU3,XINE_EVENT_INPUT_MENU4,
		XINE_EVENT_INPUT_MENU5,XINE_EVENT_INPUT_MENU6,XINE_EVENT_INPUT_MENU7,
		XINE_EVENT_INPUT_PREVIOUS,XINE_EVENT_INPUT_NEXT,
		XINE_EVENT_INPUT_UP,XINE_EVENT_INPUT_DOWN,XINE_EVENT_INPUT_LEFT,XINE_EVENT_INPUT_RIGHT,XINE_EVENT_INPUT_SELECT
		};
	
	/* Check for regular events first: */
	if(eventId<14)
		{
		/* Prepare a xine event: */
		xine_event_t event;
		event.stream=&*stream;
		event.data=0;
		event.data_length=0;
		event.type=xineEventTypes[eventId];
		
		/* Send the event: */
		xine_event_send(&*stream,&event);
		}
	else if(eventId==14)
		{
		/* Toggle pause state: */
		if(xine_get_param(&*stream,XINE_PARAM_SPEED)==XINE_SPEED_PAUSE)
			xine_set_param(&*stream,XINE_PARAM_SPEED,XINE_SPEED_NORMAL);
		else
			xine_set_param(&*stream,XINE_PARAM_SPEED,XINE_SPEED_PAUSE);
		}
	}

void VruiXine::setStereoMode(int newStereoMode)
	{
	if(stereoMode==newStereoMode)
		return;
	
	/* Change the stereo mode: */
	stereoMode=newStereoMode;
	++screenParametersVersion;
	
	/* Change the stereo layout radio box based on the new stereo mode: */
	switch(stereoMode)
		{
		case 0: // Mono
			for(int i=0;i<2;++i)
				static_cast<GLMotif::Label*>(stereoLayouts->getChild(i))->setEnabled(false);
			break;
		
		case 1: // Side-by-side
			for(int i=0;i<2;++i)
				static_cast<GLMotif::Label*>(stereoLayouts->getChild(i))->setEnabled(true);
			static_cast<GLMotif::Label*>(stereoLayouts->getChild(0))->setString("Left Eye Is Left");
			static_cast<GLMotif::Label*>(stereoLayouts->getChild(1))->setString("Left Eye Is Right");
			break;
		
		case 2: // Top/Bottom
			for(int i=0;i<2;++i)
				static_cast<GLMotif::Label*>(stereoLayouts->getChild(i))->setEnabled(true);
			static_cast<GLMotif::Label*>(stereoLayouts->getChild(0))->setString("Left Eye Is Bottom");
			static_cast<GLMotif::Label*>(stereoLayouts->getChild(1))->setString("Left Eye Is Top");
			break;
		}
	
	/* Update the stereo mode radio box: */
	stereoModes->setSelectedToggle(stereoMode);
	}

void VruiXine::setStereoLayout(int newStereoLayout)
	{
	if(stereoLayout==newStereoLayout)
		return;
	
	/* Change the stereo layout: */
	stereoLayout=newStereoLayout;
	
	/* Update the stereo layout radio box: */
	stereoLayouts->setSelectedToggle(stereoLayout);
	}

void VruiXine::setStereoSquashed(bool newStereoSquashed)
	{
	if(stereoSquashed==newStereoSquashed)
		return;
	
	/* Change the stereo squashed flag: */
	stereoSquashed=newStereoSquashed;
	++screenParametersVersion;
	
	/* Update the stereo squashed toggle: */
	stereoSquashedToggle->setToggle(stereoSquashed);
	}

void VruiXine::setForceMono(bool newForceMono)
	{
	if(forceMono==newForceMono)
		return;
	
	/* Change the force mono flag: */
	forceMono=newForceMono;
	
	/* Update the force mono toggle: */
	forceMonoToggle->setToggle(forceMono);
	}

void VruiXine::setStereoSeparation(float newStereoSeparation)
	{
	if(stereoSeparation==newStereoSeparation)
		return;
	
	/* Change the stereo separation: */
	stereoSeparation=newStereoSeparation;
	
	/* Update the stereo separation slider: */
	stereoSeparationSlider->setValue(stereoSeparation);
	}

void VruiXine::setCropLeft(int newCropLeft)
	{
	if(crop[0]==newCropLeft)
		return;
	
	/* Change the left crop: */
	crop[0]=newCropLeft;
	frameSize[0]=videoFrames.getLockedValue().size[0]-crop[0]-crop[1];
	++screenParametersVersion;
	++videoFrameVersion;
	
	/* Update the left crop slider: */
	cropSliders[0]->setValue(crop[0]);
	}

void VruiXine::setCropRight(int newCropRight)
	{
	if(crop[1]==newCropRight)
		return;
	
	/* Change the right crop: */
	crop[1]=newCropRight;
	frameSize[0]=videoFrames.getLockedValue().size[0]-crop[0]-crop[1];
	++screenParametersVersion;
	++videoFrameVersion;
	
	/* Update the right crop slider: */
	cropSliders[1]->setValue(crop[1]);
	}

void VruiXine::setCropBottom(int newCropBottom)
	{
	if(crop[2]==newCropBottom)
		return;
	
	/* Change the bottom crop: */
	crop[2]=newCropBottom;
	frameSize[1]=videoFrames.getLockedValue().size[1]-crop[2]-crop[3];
	++screenParametersVersion;
	++videoFrameVersion;
	
	/* Update the bottom crop slider: */
	cropSliders[2]->setValue(crop[2]);
	}

void VruiXine::setCropTop(int newCropTop)
	{
	if(crop[3]==newCropTop)
		return;
	
	/* Change the top crop: */
	crop[3]=newCropTop;
	frameSize[1]=videoFrames.getLockedValue().size[1]-crop[2]-crop[3];
	++screenParametersVersion;
	++videoFrameVersion;
	
	/* Update the top crop slider: */
	cropSliders[3]->setValue(crop[3]);
	}

void VruiXine::loadVideo(const char* newVideoFileName)
	{
	/* Determine the type of video file: */
	if(Misc::hasCaseExtension(newVideoFileName,".iso"))
		{
		/* Open a DVD image inside an iso file: */
		std::string mrl="dvd:/";
		mrl.append(newVideoFileName);
		if(!xine_open(&*stream,mrl.c_str())||!xine_play(&*stream,0,0))
			{
			/* Signal an error and bail out: */
			Misc::formattedUserError("Load Video: Unable to play DVD %s",newVideoFileName);
			return;
			}
		}
	else
		{
		/* Open a video file: */
		if(!xine_open(&*stream,newVideoFileName)||!xine_play(&*stream,0,0))
			{
			/* Signal an error and bail out: */
			Misc::formattedUserError("Load Video: Unable to play video file %s",newVideoFileName);
			return;
			}
		}
	
	/* Remember the video file name: */
	videoFileName=newVideoFileName;
	
	/* Check if there is a configuration file of the same base name in the same directory: */
	const char* extension=0;
	for(const char* vfnPtr=newVideoFileName;*vfnPtr!='\0';++vfnPtr)
		if(*vfnPtr=='.')
			extension=vfnPtr;
	std::string configFileName=extension!=0?std::string(newVideoFileName,extension):videoFileName;
	while(!configFileName.empty()&&configFileName[configFileName.size()-1]=='/')
		configFileName.erase(configFileName.end()-1);
	configFileName.append(".cfg");
	try
		{
		/* Try opening the configuration file: */
		Misc::ConfigurationFile cfg(configFileName.c_str());
		
		/* Read configuration settings and update the UI accordingly: */
		setStereoMode(cfg.retrieveValue<int>("./stereoMode",stereoMode));
		setStereoLayout(cfg.retrieveValue<int>("./stereoLayout",stereoLayout));
		setStereoSquashed(cfg.retrieveValue<bool>("./stereoSquashed",stereoSquashed));
		setForceMono(cfg.retrieveValue<bool>("./forceMono",forceMono));
		setStereoSeparation(cfg.retrieveValue<float>("./stereoSeparation",stereoSeparation));
		setCropLeft(cfg.retrieveValue<int>("./cropLeft",crop[0]));
		setCropRight(cfg.retrieveValue<int>("./cropRight",crop[1]));
		setCropBottom(cfg.retrieveValue<int>("./cropBottom",crop[2]));
		setCropTop(cfg.retrieveValue<int>("./cropTop",crop[3]));
		}
	catch(...)
		{
		/* Ignore the error */
		}
	
	/* Set the stream's audio volume: */
	xine_set_param(&*stream,XINE_PARAM_AUDIO_VOLUME,int(Math::floor(volumeSlider->getValue()+0.5)));
	}

void VruiXine::stereoModesValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	setStereoMode(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle));
	}

void VruiXine::stereoLayoutsValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	setStereoLayout(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle));
	}

void VruiXine::stereoSquashedToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	setStereoSquashed(cbData->set);
	}

void VruiXine::forceMonoToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	setForceMono(cbData->set);
	}

void VruiXine::stereoSeparationSliderValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Adjust the stereo separation: */
	setStereoSeparation(float(cbData->value));
	}

void VruiXine::cropSliderValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData,const int& sliderIndex)
	{
	switch(sliderIndex)
		{
		case 0:
			setCropLeft(int(Math::floor(cbData->value+0.5)));
			break;
		
		case 1:
			setCropRight(int(Math::floor(cbData->value+0.5)));
			break;
		
		case 2:
			setCropBottom(int(Math::floor(cbData->value+0.5)));
			break;
		
		case 3:
			setCropTop(int(Math::floor(cbData->value+0.5)));
			break;
		}
	}

GLMotif::PopupWindow* VruiXine::createStreamControlDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* streamControlDialogPopup=new GLMotif::PopupWindow("StreamControlDialogPopup",Vrui::getWidgetManager(),"Stream Settings");
	streamControlDialogPopup->setResizableFlags(false,false);
	
	GLMotif::RowColumn* streamControlDialog=new GLMotif::RowColumn("StreamControlDialog",streamControlDialogPopup,false);
	streamControlDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	streamControlDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	streamControlDialog->setNumMinorWidgets(2);
	
	new GLMotif::Label("StereoModesLabel",streamControlDialog,"Stereo Mode");
	
	stereoModes=new GLMotif::RadioBox("StereoModes",streamControlDialog,false);
	stereoModes->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	stereoModes->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	stereoModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	stereoModes->addToggle("Mono");
	stereoModes->addToggle("Side-by-Side");
	stereoModes->addToggle("Top/Bottom");
	
	stereoModes->setSelectedToggle(stereoMode);
	stereoModes->getValueChangedCallbacks().add(this,&VruiXine::stereoModesValueChangedCallback);
	stereoModes->manageChild();
	
	new GLMotif::Label("StereoLayoutsLabel",streamControlDialog,"Stereo Layout");
	
	GLMotif::RowColumn* stereoBox=new GLMotif::RowColumn("StereoBox",streamControlDialog,false);
	stereoBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	stereoBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	stereoLayouts=new GLMotif::RadioBox("StereoLayouts",stereoBox,false);
	stereoLayouts->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	stereoLayouts->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	stereoLayouts->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	if(stereoMode==0||stereoMode==1)
		{
		stereoLayouts->addToggle("Left Eye Is Left");
		stereoLayouts->addToggle("Left Eye Is Right");
		
		if(stereoMode==0)
			{
			for(int i=0;i<2;++i)
				static_cast<GLMotif::Label*>(stereoLayouts->getChild(i))->setEnabled(false);
			}
		}
	else
		{
		stereoLayouts->addToggle("Left Eye Is Bottom");
		stereoLayouts->addToggle("Left Eye Is Top");
		}
	
	stereoLayouts->setSelectedToggle(stereoLayout);
	stereoLayouts->getValueChangedCallbacks().add(this,&VruiXine::stereoLayoutsValueChangedCallback);
	stereoLayouts->manageChild();
	
	stereoSquashedToggle=new GLMotif::ToggleButton("StereoSquashedToggle",stereoBox,"Squashed Stereo");
	stereoSquashedToggle->setBorderWidth(0.0f);
	stereoSquashedToggle->setToggleType(GLMotif::ToggleButton::TOGGLE_BUTTON);
	stereoSquashedToggle->setToggle(stereoSquashed);
	stereoSquashedToggle->getValueChangedCallbacks().add(this,&VruiXine::stereoSquashedToggleValueChangedCallback);
	
	forceMonoToggle=new GLMotif::ToggleButton("ForceMonoToggle",stereoBox,"Force Mono");
	forceMonoToggle->setBorderWidth(0.0f);
	forceMonoToggle->setToggleType(GLMotif::ToggleButton::TOGGLE_BUTTON);
	forceMonoToggle->setToggle(forceMono);
	forceMonoToggle->getValueChangedCallbacks().add(this,&VruiXine::forceMonoToggleValueChangedCallback);
	
	stereoBox->manageChild();
	
	new GLMotif::Label("StereoSeparatoinLabel",streamControlDialog,"Stereo Separation");
	
	stereoSeparationSlider=new GLMotif::TextFieldSlider("StereoSeparationSlider",streamControlDialog,6,ss.fontHeight*10.0f);
	stereoSeparationSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	stereoSeparationSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	stereoSeparationSlider->setValueRange(-0.02,0.02,0.0002);
	stereoSeparationSlider->getSlider()->addNotch(0.0f);
	stereoSeparationSlider->setValue(stereoSeparation);
	stereoSeparationSlider->getValueChangedCallbacks().add(this,&VruiXine::stereoSeparationSliderValueChangedCallback);
	
	new GLMotif::Label("CropLabel",streamControlDialog,"Crop");
	
	GLMotif::RowColumn* cropBox=new GLMotif::RowColumn("CropBox",streamControlDialog,false);
	cropBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	cropBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	cropBox->setNumMinorWidgets(2);
	
	static const char* cropLabelNames[4]={"CropLeftLabel","CropRightLabel","CropBottomLabel","CropTopLabel"};
	static const char* cropLabelLabels[4]={"Left","Right","Bottom","Top"};
	static const char* cropSliderNames[4]={"CropLeftSlider","CropRightSlider","CropBottomSlider","CropTopSlider"};
	
	for(int i=0;i<4;++i)
		{
		new GLMotif::Label(cropLabelNames[i],cropBox,cropLabelLabels[i]);
		
		cropSliders[i]=new GLMotif::TextFieldSlider(cropSliderNames[i],cropBox,5,ss.fontHeight*6.0f);
		cropSliders[i]->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
		cropSliders[i]->setValueType(GLMotif::TextFieldSlider::INT);
		cropSliders[i]->setValueRange(0.0,10.0,1.0);
		cropSliders[i]->setValue(crop[i]);
		cropSliders[i]->getValueChangedCallbacks().add(this,&VruiXine::cropSliderValueChangedCallback,i);
		}
	
	cropBox->manageChild();
	
	streamControlDialog->manageChild();
	
	return streamControlDialogPopup;
	}

void VruiXine::dvdNavigationButtonCallback(Misc::CallbackData*,const int& eventId)
	{
	/* Send an event to the xine library: */
	xineSendEvent(eventId);
	}

void VruiXine::loadVideoFileCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	loadVideo(cbData->getSelectedPath().c_str());
	}

void VruiXine::saveConfigurationCallback(Misc::CallbackData*)
	{
	/* Create a configuration file of the same base name as the video file: */
	const char* extension=0;
	for(const char* vfnPtr=videoFileName.c_str();*vfnPtr!='\0';++vfnPtr)
		if(*vfnPtr=='.')
			extension=vfnPtr;
	std::string configFileName=extension!=0?std::string(videoFileName.c_str(),extension):videoFileName;
	while(!configFileName.empty()&&configFileName[configFileName.size()-1]=='/')
		configFileName.erase(configFileName.end()-1);
	configFileName.append(".cfg");
	try
		{
		/* Try creating the configuration file: */
		Misc::ConfigurationFile cfg;
		
		/* Write configuration settings: */
		cfg.storeValue<int>("./stereoMode",stereoMode);
		cfg.storeValue<int>("./stereoLayout",stereoLayout);
		cfg.storeValue<bool>("./stereoSquashed",stereoSquashed);
		cfg.storeValue<bool>("./forceMono",forceMono);
		cfg.storeValue<float>("./stereoSeparation",stereoSeparation);
		cfg.storeValue<int>("./cropLeft",crop[0]);
		cfg.storeValue<int>("./cropRight",crop[1]);
		cfg.storeValue<int>("./cropBottom",crop[2]);
		cfg.storeValue<int>("./cropTop",crop[3]);
		
		cfg.saveAs((configFileName.c_str()));
		}
	catch(std::runtime_error err)
		{
		/* Signal an error: */
		Misc::formattedUserError("Save Configuration: Unable to save configuration for video file %s due to exception %s",videoFileName.c_str(),err.what());
		}
	}

void VruiXine::volumeSliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
	{
	/* Set the stream's audio volume: */
	xine_set_param(&*stream,XINE_PARAM_AUDIO_VOLUME,int(Math::floor(cbData->value+0.5)));
	}

GLMotif::PopupWindow* VruiXine::createDvdNavigationDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* dvdNavigationDialogPopup=new GLMotif::PopupWindow("DvdNavigationDialogPopup",Vrui::getWidgetManager(),"DVD Navigation");
	dvdNavigationDialogPopup->setResizableFlags(false,false);
	
	GLMotif::RowColumn* dvdNavigationDialog1=new GLMotif::RowColumn("DvdNavigationDialog2",dvdNavigationDialogPopup,false);
	dvdNavigationDialog1->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	dvdNavigationDialog1->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	GLMotif::RowColumn* dvdNavigationDialog2=new GLMotif::RowColumn("DvdNavigationDialog2",dvdNavigationDialog1,false);
	dvdNavigationDialog2->setOrientation(GLMotif::RowColumn::VERTICAL);
	dvdNavigationDialog2->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	GLMotif::RowColumn* menuPanel=new GLMotif::RowColumn("MenuPanel",dvdNavigationDialog2,false);
	menuPanel->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	menuPanel->setPacking(GLMotif::RowColumn::PACK_GRID);
	menuPanel->setNumMinorWidgets(1);
	
	GLMotif::Button* menu1Button=new GLMotif::Button("Menu1Button",menuPanel,"Menu 1");
	menu1Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,0);
	
	GLMotif::Button* menu2Button=new GLMotif::Button("Menu2Button",menuPanel,"Menu 2");
	menu2Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,1);
	
	GLMotif::Button* menu3Button=new GLMotif::Button("Menu3Button",menuPanel,"Menu 3");
	menu3Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,2);
	
	GLMotif::Button* menu4Button=new GLMotif::Button("Menu4Button",menuPanel,"Menu 4");
	menu4Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,3);
	
	GLMotif::Button* menu5Button=new GLMotif::Button("Menu5Button",menuPanel,"Menu 5");
	menu5Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,4);
	
	GLMotif::Button* menu6Button=new GLMotif::Button("Menu6Button",menuPanel,"Menu 6");
	menu6Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,5);
	
	GLMotif::Button* menu7Button=new GLMotif::Button("Menu7Button",menuPanel,"Menu 7");
	menu7Button->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,6);
	
	menuPanel->manageChild();
	
	GLMotif::RowColumn* lowerPanel=new GLMotif::RowColumn("LowerPanel",dvdNavigationDialog2,false);
	lowerPanel->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	lowerPanel->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	lowerPanel->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* menuNavPanel=new GLMotif::RowColumn("MenuNavPanel",lowerPanel,false);
	menuNavPanel->setOrientation(GLMotif::RowColumn::VERTICAL);
	menuNavPanel->setPacking(GLMotif::RowColumn::PACK_GRID);
	menuNavPanel->setNumMinorWidgets(3);
	
	new GLMotif::Blind("TopLeftBlind",menuNavPanel);
	
	GLMotif::NewButton* upButton=new GLMotif::NewButton("UpButton",menuNavPanel);
	upButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,9);
	new GLMotif::Glyph("UpGlyph",upButton,GLMotif::GlyphGadget::FANCY_ARROW_UP,GLMotif::GlyphGadget::IN);
	
	new GLMotif::Blind("TopRightBlind",menuNavPanel);
	
	GLMotif::NewButton* leftButton=new GLMotif::NewButton("LeftButton",menuNavPanel);
	leftButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,11);
	new GLMotif::Glyph("LeftGlyph",leftButton,GLMotif::GlyphGadget::FANCY_ARROW_LEFT,GLMotif::GlyphGadget::IN);
	
	GLMotif::NewButton* selectButton=new GLMotif::NewButton("SelectButton",menuNavPanel,"OK");
	selectButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,13);
	
	GLMotif::NewButton* rightButton=new GLMotif::NewButton("RightButton",menuNavPanel);
	rightButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,12);
	new GLMotif::Glyph("RightGlyph",rightButton,GLMotif::GlyphGadget::FANCY_ARROW_RIGHT,GLMotif::GlyphGadget::IN);
	
	new GLMotif::Blind("BottomLeftBlind",menuNavPanel);
	
	GLMotif::NewButton* downButton=new GLMotif::NewButton("DownButton",menuNavPanel);
	downButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,10);
	new GLMotif::Glyph("DownGlyph",downButton,GLMotif::GlyphGadget::FANCY_ARROW_DOWN,GLMotif::GlyphGadget::IN);
	
	new GLMotif::Blind("BottomRightBlind",menuNavPanel);
	
	menuNavPanel->manageChild();
	
	GLMotif::Margin* chapterNavMargin=new GLMotif::Margin("ChapterNavMargin",lowerPanel,false);
	chapterNavMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER,GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* chapterNavPanel=new GLMotif::RowColumn("ChapterNavPanel",chapterNavMargin,false);
	chapterNavPanel->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	chapterNavPanel->setPacking(GLMotif::RowColumn::PACK_GRID);
	chapterNavPanel->setNumMinorWidgets(1);
	
	GLMotif::Button* prevButton=new GLMotif::Button("PrevButton",chapterNavPanel,"Prev");
	prevButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,7);
	
	GLMotif::Button* pauseButton=new GLMotif::Button("PauseButton",chapterNavPanel,"Pause");
	pauseButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,14);
	
	GLMotif::Button* nextButton=new GLMotif::Button("NextButton",chapterNavPanel,"Next");
	nextButton->getSelectCallbacks().add(this,&VruiXine::dvdNavigationButtonCallback,8);
	
	chapterNavPanel->manageChild();
	
	chapterNavMargin->manageChild();
	
	lowerPanel->manageChild();
	
	GLMotif::Margin* fileMargin=new GLMotif::Margin("FileMargin",dvdNavigationDialog2,false);
	fileMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT,GLMotif::Alignment::VFILL));
	
	GLMotif::RowColumn* fileBox=new GLMotif::RowColumn("FileBox",fileMargin,false);
	fileBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	fileBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	fileBox->setNumMinorWidgets(1);
	
	GLMotif::Button* ejectButton=new GLMotif::Button("EjectButton",fileBox,"Eject");
	videoFileSelectionHelper.addLoadCallback(ejectButton,Misc::createFunctionCall(this,&VruiXine::loadVideoFileCallback));
	
	GLMotif::Button* saveConfigButton=new GLMotif::Button("SaveConfigButton",fileBox,"Save Configuration");
	saveConfigButton->getSelectCallbacks().add(this,&VruiXine::saveConfigurationCallback);
	
	fileBox->manageChild();
	
	fileMargin->manageChild();
	
	dvdNavigationDialog2->manageChild();
	
	volumeSlider=new GLMotif::Slider("VolumeSlider",dvdNavigationDialog1,GLMotif::Slider::VERTICAL,ss.fontHeight*5.0f);
	volumeSlider->setValueRange(0.0,100.0,1.0);
	volumeSlider->getValueChangedCallbacks().add(this,&VruiXine::volumeSliderValueChangedCallback);
	volumeSlider->setValue(0.0);
	
	dvdNavigationDialog1->manageChild();
	
	return dvdNavigationDialogPopup;
	}

namespace {

/****************
Helper functions:
****************/

inline char* formatTime(int timeMs,char timeString[9])
	{
	int t=(timeMs+500)/1000;
	int seconds=t%60;
	t/=60;
	int minutes=t%60;
	t/=60;
	int hours=t;
	char* tsPtr=timeString;
	*(tsPtr++)=char(hours/10+'0');
	*(tsPtr++)=char(hours%10+'0');
	*(tsPtr++)=':';
	*(tsPtr++)=char(minutes/10+'0');
	*(tsPtr++)=char(minutes%10+'0');
	*(tsPtr++)=':';
	*(tsPtr++)=char(seconds/10+'0');
	*(tsPtr++)=char(seconds%10+'0');
	*(tsPtr++)='\0';
	
	return timeString;
	}

}

void VruiXine::skipBackCallback(Misc::CallbackData*)
	{
	/* Skip back by 10 seconds: */
	double newTime=playbackSlider->getValue()-10.0;
	xine_play(&*stream,0,int(newTime*1000.0+0.5));
	
	/* Invalidate the playback position: */
	playbackPosCheckTime=0.0;
	}

void VruiXine::skipAheadCallback(Misc::CallbackData*)
	{
	/* Skip ahead by 10 seconds: */
	double newTime=playbackSlider->getValue()+10.0;
	xine_play(&*stream,0,int(newTime*1000.0+0.5));
	
	/* Invalidate the playback position: */
	playbackPosCheckTime=0.0;
	}

void VruiXine::playbackSliderDraggingCallback(GLMotif::DragWidget::DraggingCallbackData* cbData)
	{
	if(cbData->reason==GLMotif::DragWidget::DraggingCallbackData::DRAGGING_STOPPED)
		{
		/* Set the playback position in the current stream: */
		xine_play(&*stream,0,int(playbackSlider->getValue()*1000.0+0.5));
		
		/* Invalidate the playback position: */
		playbackPosCheckTime=0.0;
		}
	}

void VruiXine::microPlaybackSliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
	gotoRecorded(cbData->value);
}

void VruiXine::playbackSliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
	{
	/* Update the playback position text field to the current slider position: */
	char timeString[9];
	playbackPositionText->setString(formatTime(int(cbData->value*1000.0+0.5),timeString));
	
	/* Don't update the playback position: */
	playbackPosCheckTime=Vrui::getApplicationTime()+1000.0;
	}

GLMotif::PopupWindow* VruiXine::createPlaybackControlDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* playbackControlDialogPopup=new GLMotif::PopupWindow("PlaybackControlDialogPopup",Vrui::getWidgetManager(),"Playback Control");
	playbackControlDialogPopup->setResizableFlags(true,false);
	

	GLMotif::RowColumn * column = new GLMotif::RowColumn("EditorControlDialog", playbackControlDialogPopup, false);

	{
		GLMotif::RowColumn* playbackControlDialog=new GLMotif::RowColumn("PlaybackControlDialog",column,false);
		playbackControlDialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
		playbackControlDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		
		playbackPositionText=new GLMotif::TextField("PlaybackPositionText",playbackControlDialog,9);
		playbackPositionText->setString("00:00:00");
		
		GLMotif::NewButton* skipBackButton=new GLMotif::NewButton("SkipBackButton",playbackControlDialog);
		skipBackButton->getSelectCallbacks().add(this,&VruiXine::skipBackCallback);
		new GLMotif::Glyph("LeftGlyph",skipBackButton,GLMotif::GlyphGadget::SIMPLE_ARROW_LEFT,GLMotif::GlyphGadget::IN);
		
		playbackSlider=new GLMotif::Slider("PlaybackSlider",playbackControlDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*25.0f);
		playbackSlider->setValueRange(0.0,1.0,1.0);
		playbackSlider->setValue(0.0);
		playbackSlider->getDraggingCallbacks().add(this,&VruiXine::playbackSliderDraggingCallback);
		playbackSlider->getValueChangedCallbacks().add(this,&VruiXine::playbackSliderValueChangedCallback);
		
		GLMotif::NewButton* skipAheadButton=new GLMotif::NewButton("SkipAheadButton",playbackControlDialog);
		skipAheadButton->getSelectCallbacks().add(this,&VruiXine::skipAheadCallback);
		new GLMotif::Glyph("RightGlyph",skipAheadButton,GLMotif::GlyphGadget::SIMPLE_ARROW_RIGHT,GLMotif::GlyphGadget::IN);
		
		streamLengthText=new GLMotif::TextField("StreamLengthText",playbackControlDialog,9);
		streamLengthText->setString("00:00:00");
		
		playbackControlDialog->setColumnWeight(1,1.0);
		
		playbackControlDialog->manageChild();
	}


	{
		GLMotif::RowColumn* microRow=new GLMotif::RowColumn("MicroRow",column,false);
		microRow->setOrientation(GLMotif::RowColumn::HORIZONTAL);
		microRow->setPacking(GLMotif::RowColumn::PACK_TIGHT);

		microPlaybackSlider=new GLMotif::Slider("PlaybackSlider",microRow,GLMotif::Slider::HORIZONTAL,ss.fontHeight*25.0f);
		microPlaybackSlider->setValueRange(0, recordedFrames.size() - 1, 1);
		microPlaybackSlider->setValue(0);
		microPlaybackSlider->getValueChangedCallbacks().add(this,&VruiXine::microPlaybackSliderValueChangedCallback);

		microRow->manageChild();
	}


	column->manageChild();
	
	return playbackControlDialogPopup;
	}

void VruiXine::screenDistanceValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the screen distance from the display center: */
	screenCenter[1]=cbData->value;
	
	++screenParametersVersion;
	}

void VruiXine::screenHeightValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Get the current screen bottom elevation: */
	Vrui::Scalar screenBottom=screenCenter[2]-Math::div2(screenHeight);
	
	/* Set the screen height: */
	screenHeight=cbData->value;
	
	/* Update the screen center: */
	screenCenter[2]=screenBottom+Math::div2(screenHeight);
	
	++screenParametersVersion;
	}

void VruiXine::screenBottomValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the screen center: */
	screenCenter[2]=cbData->value+Math::div2(screenHeight);
	
	++screenParametersVersion;
	}

void VruiXine::screenAzimuthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set screen's azimuth angle: */
	screenAzimuth=cbData->value;
	}

GLMotif::PopupWindow* VruiXine::createScreenControlDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* screenControlDialogPopup=new GLMotif::PopupWindow("ScreenControlDialogPopup",Vrui::getWidgetManager(),"Screen Control");
	screenControlDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* screenControlDialog=new GLMotif::RowColumn("ScreenControlDialog",screenControlDialogPopup,false);
	screenControlDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	screenControlDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	screenControlDialog->setNumMinorWidgets(2);
	
	new GLMotif::Label("ScreenDistanceLabel",screenControlDialog,"Screen Distance");
	
	GLMotif::TextFieldSlider* screenDistanceSlider=new GLMotif::TextFieldSlider("ScreenDistanceSlider",screenControlDialog,5,ss.fontHeight*10.0f);
	screenDistanceSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	screenDistanceSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	screenDistanceSlider->getTextField()->setPrecision(1);
	screenDistanceSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	screenDistanceSlider->setValueRange(1.0,20.0,0.1);
	screenDistanceSlider->setValue(screenCenter[1]);
	screenDistanceSlider->getValueChangedCallbacks().add(this,&VruiXine::screenDistanceValueChangedCallback);
	
	new GLMotif::Label("ScreenHeightLabel",screenControlDialog,"Screen Height");
	
	GLMotif::TextFieldSlider* screenHeightSlider=new GLMotif::TextFieldSlider("ScreenHeightSlider",screenControlDialog,5,ss.fontHeight*10.0f);
	screenHeightSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	screenHeightSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	screenHeightSlider->getTextField()->setPrecision(1);
	screenHeightSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	screenHeightSlider->setValueRange(0.5,20.0,0.1);
	screenHeightSlider->setValue(screenHeight);
	screenHeightSlider->getValueChangedCallbacks().add(this,&VruiXine::screenHeightValueChangedCallback);
	
	new GLMotif::Label("ScreenBottomLabel",screenControlDialog,"Screen Bottom");
	
	GLMotif::TextFieldSlider* screenBottomSlider=new GLMotif::TextFieldSlider("ScreenBottomSlider",screenControlDialog,5,ss.fontHeight*10.0f);
	screenBottomSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	screenBottomSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	screenBottomSlider->getTextField()->setPrecision(1);
	screenBottomSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
	screenBottomSlider->setValueRange(-10.0,10.0,0.1);
	screenBottomSlider->setValue(screenCenter[2]-Math::div2(screenHeight));
	screenBottomSlider->getValueChangedCallbacks().add(this,&VruiXine::screenBottomValueChangedCallback);
	
	new GLMotif::Label("ScreenRotationLabel",screenControlDialog,"Screen Rotation");
	
	GLMotif::RowColumn* screenRotationBox=new GLMotif::RowColumn("ScreenRotationBox",screenControlDialog,false);
	screenRotationBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	screenRotationBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	new GLMotif::Label("ScreenAzimuthLabel",screenRotationBox,"Azimuth");
	
	GLMotif::TextFieldSlider* screenAzimuthSlider=new GLMotif::TextFieldSlider("ScreenAzimuthSlider",screenRotationBox,5,ss.fontHeight*25.0f);
	screenAzimuthSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	screenAzimuthSlider->setValueType(GLMotif::TextFieldSlider::INT);
	screenAzimuthSlider->setValueRange(-180.0,180.0,1.0);
	screenAzimuthSlider->setValue(screenAzimuth);
	screenAzimuthSlider->getSlider()->addNotch(0.0f);
	screenAzimuthSlider->getValueChangedCallbacks().add(this,&VruiXine::screenAzimuthValueChangedCallback);
	
	screenRotationBox->setColumnWeight(1,1.0f);
	screenRotationBox->setColumnWeight(3,1.0f);
	screenRotationBox->manageChild();
	
	screenControlDialog->manageChild();
	
	return screenControlDialogPopup;
	}

VruiXine::VruiXine(std::vector<std::string> const & args)
	:videoFileSelectionHelper(Vrui::getWidgetManager(),"",".mp4;.iso",Vrui::openDirectory(".")),
	 videoFrameVersion(0),overlaySetVersion(0),
	 playbackPosCheckTime(0.0),streamLength(0.0),
	 aspectRatio(Vrui::Scalar(16.0/9.0)),
	 stereoMode(0),stereoLayout(0),stereoSquashed(false),forceMono(false),stereoSeparation(0.0f),
	 screenParametersVersion(1)
	{
	frameSize[0]=frameSize[1]=0;
	
	/* Parse the command line: */
	const char* mrl=0;
	crop[3]=crop[2]=crop[1]=crop[0]=0;
	bool startPaused=false;

	int const argc(args.size());
	std::vector<char const *> argvec;
	for (std::string const & a : args) argvec.push_back(a.c_str());
	char const ** argv = argvec.data();

	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"sideBySide")==0||strcasecmp(argv[i]+1,"sbs")==0)
				{
				std::cout<<"Enabling side-by-side stereo"<<std::endl;
				stereoMode=1;
				}
			else if(strcasecmp(argv[i]+1,"topBottom")==0||strcasecmp(argv[i]+1,"tb")==0)
				{
				std::cout<<"Enabling top/bottom stereo"<<std::endl;
				stereoMode=2;
				}
			else if(strcasecmp(argv[i]+1,"flipStereo")==0||strcasecmp(argv[i]+1,"fs")==0)
				stereoLayout=1;
			else if(strcasecmp(argv[i]+1,"squashedStereo")==0||strcasecmp(argv[i]+1,"ss")==0)
				stereoSquashed=true;
			else if(strcasecmp(argv[i]+1,"crop")==0)
				{
				for(int j=0;j<4&&i<argc;++j)
					crop[j]=atoi(argv[i+1+j]);
				i+=4;
				}
			else if(strcasecmp(argv[i]+1,"paused")==0||strcasecmp(argv[i]+1,"p")==0)
				startPaused=true;
			else
				std::cerr<<"Unknown option "<<argv[i]<<" ignored"<<std::endl;
			}
		else if(mrl==0)
			mrl=argv[i];
		else
			std::cerr<<"Additional stream MRL "<<argv[i]<<" ignored"<<std::endl;
		}
	
	if(mrl==0)
		throw std::runtime_error("VruiXine: No stream MRL provided");
	
	/* Create an abstract base class for application tool classes: */
	typedef Vrui::GenericAbstractToolFactory<Vrui::Application::Tool<VruiXine> > BaseToolFactory;
	BaseToolFactory* baseToolFactory=new BaseToolFactory("BaseTool","VruiXine",0,*Vrui::getToolManager());
	Vrui::getToolManager()->addAbstractClass(baseToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Create an abstract base class for DVD menu tool classes: */
	BaseToolFactory* dvdMenuToolFactory=new BaseToolFactory("DVDMenuTool","DVD Menu",baseToolFactory,*Vrui::getToolManager());
	Vrui::getToolManager()->addAbstractClass(dvdMenuToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Create dvd menu event tool classes: */
/*
	addEventTool("Menu 1",dvdMenuToolFactory,0);
	addEventTool("Menu 2",dvdMenuToolFactory,1);
	addEventTool("Menu 3",dvdMenuToolFactory,2);
	addEventTool("Menu 4",dvdMenuToolFactory,3);
	addEventTool("Menu 5",dvdMenuToolFactory,4);
	addEventTool("Menu 6",dvdMenuToolFactory,5);
	addEventTool("Menu 7",dvdMenuToolFactory,6);
*/
	/* Create an abstract base class for DVD menu navigation tool classes: */
	BaseToolFactory* dvdMenuNavToolFactory=new BaseToolFactory("DVDMenuNavTool","DVD Menu Navigation",baseToolFactory,*Vrui::getToolManager());
	Vrui::getToolManager()->addAbstractClass(dvdMenuNavToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Create dvd menu navigation event tool classes: */
/*
	addEventTool("Up",dvdMenuNavToolFactory,9);
	addEventTool("Down",dvdMenuNavToolFactory,10);
	addEventTool("Left",dvdMenuNavToolFactory,11);
	addEventTool("Right",dvdMenuNavToolFactory,12);
	addEventTool("Select",dvdMenuNavToolFactory,13);
*/
	/* Create an abstract base class for DVD chapter navigation tool classes: */
	BaseToolFactory* dvdChapterNavToolFactory=new BaseToolFactory("DVDChapterNavTool","DVD Chapter Navigation",baseToolFactory,*Vrui::getToolManager());
	Vrui::getToolManager()->addAbstractClass(dvdChapterNavToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Create dvd chapter navigation event tool classes: */
//	addEventTool("Previous",dvdChapterNavToolFactory,7);
//	addEventTool("Next",dvdChapterNavToolFactory,8);
	
	/* Create pause toggle event tool class: */
//	addEventTool("Pause",baseToolFactory,14);
	
	/* Create the stream control dialog: */
	streamControlDialog.reset(createStreamControlDialog());
	Vrui::popupPrimaryWidget(streamControlDialog.get());
	
	/* Create the DVD navigation dialog: */
	dvdNavigationDialog.reset(createDvdNavigationDialog());
	Vrui::popupPrimaryWidget(dvdNavigationDialog.get());
	
	/* Create the playback control dialog: */
	playbackControlDialog.reset(createPlaybackControlDialog());
	Vrui::popupPrimaryWidget(playbackControlDialog.get());
	
	/* Create the screen control dialog: */
	screenControlDialog.reset(createScreenControlDialog());
	Vrui::popupPrimaryWidget(screenControlDialog.get());
	
	xine_event_create_listener_thread(&*eventQueue,xineEventCallback,this);
	
	/* Open the stream MRL: */
	loadVideo(mrl);
	
	if(startPaused)
		xine_set_param(&*stream,XINE_PARAM_SPEED,XINE_SPEED_PAUSE);
	
	/* Get the stream's audio volume: */
	volumeSlider->setValue(xine_get_param(&*stream,XINE_PARAM_AUDIO_VOLUME));

	Vrui::inhibitScreenSaver();
	
	/* Set the navigational coordinate unit to meters: */
	Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::METER,1.0));
	}

void VruiXine::frame(void)
	{
	/* Lock the most recent frame in the video frame buffer: */
	if(videoFrames.lockNewValue())
		{
		/* Invalidate the current video frame texture: */
		++videoFrameVersion;
		
		/* Check if the screen format changed: */
		const Frame& f=videoFrames.getLockedValue();
		if(frameSize[0]!=f.size[0]-crop[0]-crop[1]||frameSize[1]!=f.size[1]-crop[2]-crop[3]||aspectRatio!=f.aspectRatio)
			{
			++screenParametersVersion;
			frameSize[0]=f.size[0]-crop[0]-crop[1];
			frameSize[1]=f.size[1]-crop[2]-crop[3];
			aspectRatio=videoFrames.getLockedValue().aspectRatio;
			}
		}
	
	/* Lock the most recent overlay set: */
	if(overlaySets.lockNewValue())
		{
		/* Invalidate the current overlay textures: */
		++overlaySetVersion;
		}
	
	if(Vrui::getApplicationTime()>=playbackPosCheckTime)
		{
		/* Check the stream length and playback position: */
		int streamPos,streamTimeMs,streamLengthMs;
		if(xine_get_pos_length(&*stream,&streamPos,&streamTimeMs,&streamLengthMs))
			{
			/* Recalculate stream time in ms because it's bogus as reported: */
			streamTimeMs=int((long(streamLengthMs)*long(streamPos)+32768L)/65536L);
			
			/* Update the playback position text field: */
			char timeString[9];
			playbackPositionText->setString(formatTime(streamTimeMs,timeString));
			
			if(streamLength!=double(streamLengthMs)/1000.0)
				{
				streamLength=double(streamLengthMs)/1000.0;
				
				/* Update the stream length text field: */
				char timeString[9];
				streamLengthText->setString(formatTime(streamLengthMs,timeString));
				
				/* Adjust the playback slider's value range: */
				playbackSlider->setValueRange(0.0,streamLength,1.0);
				}
			
			/* Adjust the playback slider's position: */
			playbackSlider->setValue(double(streamTimeMs)/1000.0);
			}
		
		/* Check again in one second: */
		playbackPosCheckTime=Vrui::getApplicationTime()+1.0;
		}
	}

raw_visual_t VruiXine::mkRawVisual()
{
	raw_visual_t r;
	r.user_data = this;
	r.supported_formats = XINE_VORAW_YV12 | XINE_VORAW_RGB;
	r.raw_output_cb = xineOutputCallback;
	r.raw_overlay_cb = xineOverlayCallback;
	return r;
}

void VruiXine::display(GLContextData& contextData, GLObject const * glObj) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(glObj);
	
	/* Handle the current video frame based on its format: */
	const Frame& frame=videoFrames.getLockedValue();
	int shaderIndex=0;
	if(frame.format==XINE_VORAW_YV12)
		{
		/* Bind the YV12 shader: */
		shaderIndex=0;
		dataItem->videoShaders[shaderIndex].useProgram();
		
		/* Bind the Y texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[0]);
		glUniform1iARB(dataItem->videoShaderUniforms[shaderIndex][2],0);
		if(dataItem->frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0],frameSize[1],0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[0].data());
			}
		
		/* Bind the U texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[1]);
		glUniform1iARB(dataItem->videoShaderUniforms[shaderIndex][3],1);
		if(dataItem->frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]>>1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]>>1);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]>>1);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0]>>1,frameSize[1]>>1,0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[1].data());
			}
		
		/* Bind the V texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[2]);
		glUniform1iARB(dataItem->videoShaderUniforms[shaderIndex][4],2);
		if(dataItem->frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[3]>>1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]>>1);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]>>1);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R8,frameSize[0]>>1,frameSize[1]>>1,0,GL_RED,GL_UNSIGNED_BYTE,frame.planes[2].data());
			}
		
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		}
	else if(frame.format==XINE_VORAW_YUY2)
		{
		/* Bind the interleaved YU/Y2 texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[0]);
		if(dataItem->frameTextureVersion!=videoFrameVersion)
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
		dataItem->videoShaders[shaderIndex].useProgram();
		
		/* Bind the RGB texture plane: */
		glActiveTextureARB(GL_TEXTURE0_ARB+0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->frameTextureIds[0]);
		glUniform1iARB(dataItem->videoShaderUniforms[shaderIndex][2],0);
		if(dataItem->frameTextureVersion!=videoFrameVersion)
			{
			glPixelStorei(GL_UNPACK_SKIP_ROWS,crop[2]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,frame.size[0]);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS,crop[0]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB8,frameSize[0],frameSize[1],0,GL_RGB,GL_UNSIGNED_BYTE,frame.planes[0].data());
			}
		}
	
	/* Mark the texture objects as up-to-date: */
	dataItem->frameTextureVersion=videoFrameVersion;
	
	/* Set up a texture transformation: */
	int eyeIndex=stereoMode==0||forceMono?0:Vrui::getDisplayState(contextData).eyeIndex;
	GLfloat texScale[4]={1.0f,1.0f,0.0f,0.0f};
	GLfloat texOffset[4]={0.0f,0.0f,0.0f,0.0f};
	if(stereoMode==1) // Side-by-side stereo
		{
		texScale[0]=0.5f;
		texOffset[0]=eyeIndex==0?0.0f:GLfloat(frameSize[0])*0.5f;
		if(stereoLayout==1)
			texOffset[0]=GLfloat(frameSize[0])*0.5f-texOffset[0];
		}
	else if(stereoMode==2) // Top/bottom stereo
		{
		//const Vrui::DisplayState& ds=Vrui::getDisplayState(contextData);
		texScale[1]=0.5f;
		texOffset[1]=eyeIndex==0?0.0f:GLfloat(frameSize[1])*0.5f;
		if(stereoLayout==1)
			texOffset[1]=GLfloat(frameSize[1])*0.5f-texOffset[1];
		}
	texOffset[0]+=(eyeIndex==0?stereoSeparation:-stereoSeparation)*GLfloat(frameSize[0]);
	glUniform4fvARB(dataItem->videoShaderUniforms[shaderIndex][0],1,texScale);
	glUniform4fvARB(dataItem->videoShaderUniforms[shaderIndex][1],1,texOffset);
	
	/* Bind the vertex and index buffers to render the video screen: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
	
	/* Check if the vertex data is outdated: */
	if(dataItem->bufferDataVersion!=screenParametersVersion)
		{
		/* Re-generate the vertex buffer contents: */
		Vertex* vertices=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		Vertex* vPtr=vertices;

		Vrui::Scalar screenWidth=screenHeight*aspectRatio;
		if(!stereoSquashed)
		{
			if(stereoMode==1)
				screenWidth*=Vrui::Scalar(0.5);
			else if(stereoMode==2)
				screenWidth*=Vrui::Scalar(2);
		}

		/* Generate a flat screen: */
		Vrui::Scalar const
			x0=screenCenter[0]-Math::div2(screenWidth),
			z0=screenCenter[2]-Math::div2(screenHeight);
		
		GLfloat const
			tsx=GLfloat(frameSize[0])/GLfloat(dataItem->numVertices[0]-1),
			tsy=GLfloat(frameSize[1])/GLfloat(dataItem->numVertices[1]-1);

		Vrui::Scalar const
			sx=screenWidth/Vrui::Scalar(dataItem->numVertices[0]-1),
			sz=screenHeight/Vrui::Scalar(dataItem->numVertices[1]-1);

		for(int y=0;y<dataItem->numVertices[1];++y)
			for(int x=0;x<dataItem->numVertices[0];++x,++vPtr)
				{
				vPtr->texCoord[0]=GLfloat(x)*tsx;
				vPtr->texCoord[1]=GLfloat(frameSize[1])-GLfloat(y)*tsy;
				
				vPtr->position[0]=GLfloat(screenCenter[1]);
				vPtr->position[1]=GLfloat(z0+Vrui::Scalar(y)*sz);
				vPtr->position[2]=GLfloat(x0+Vrui::Scalar(x)*sx);
				}

		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		dataItem->bufferDataVersion=screenParametersVersion;
		}
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferId);
	
	/* Draw the screen: */
	glPushMatrix();
	glRotated(-screenAzimuth,0.0,1.0,0.0);
	glTranslated(4 /* todo: one of the slided values */, 0, 0);
	
	GLVertexArrayParts::enable(Vertex::getPartsMask());
	glVertexPointer(static_cast<const Vertex*>(0));
	const GLushort* indexPtr=0;
	for(int y=1;y<dataItem->numVertices[1];++y,indexPtr+=dataItem->numVertices[0]*2)
		glDrawElements(GL_QUAD_STRIP,dataItem->numVertices[0]*2,GL_UNSIGNED_SHORT,indexPtr);
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
	
	/* Draw all current overlays: */
	if(overlaySets.getLockedValue().numOverlays>0)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
		Vrui::Scalar screenWidth=screenHeight*aspectRatio;
		Vrui::Scalar x0=screenCenter[0]-Math::div2(screenWidth);
		Vrui::Scalar z0=screenCenter[2]-Math::div2(screenHeight);
		Vrui::Scalar z1=z0+screenHeight;
		Vrui::Scalar xs=screenWidth/Vrui::Scalar(frameSize[0]);
		Vrui::Scalar ys=screenHeight/Vrui::Scalar(frameSize[1]);
		const OverlaySet& ovls=overlaySets.getLockedValue();
		for(int i=0;i<ovls.numOverlays;++i)
			{
			/* Bind the overlay's texture: */
			const raw_overlay_t& ovl=ovls.overlays[i];
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->overlayTextureIds[i]);
			if(dataItem->overlayTextureVersion!=overlaySetVersion)
				glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,ovl.ovl_w,ovl.ovl_h,0,GL_RGBA,GL_UNSIGNED_BYTE,ovl.ovl_rgba);
			
			/* Draw the overlay: */
			glBegin(GL_QUADS);
			glTexCoord2i(0,ovl.ovl_h);
			glVertex3d(x0+Vrui::Scalar(ovl.ovl_x)*xs,screenCenter[1]-0.1,z1-Vrui::Scalar(ovl.ovl_y+ovl.ovl_h)*ys);
			glTexCoord2i(ovl.ovl_w,ovl.ovl_h);
			glVertex3d(x0+Vrui::Scalar(ovl.ovl_x+ovl.ovl_w)*xs,screenCenter[1]-0.1,z1-Vrui::Scalar(ovl.ovl_y+ovl.ovl_h)*ys);
			glTexCoord2i(ovl.ovl_w,0);
			glVertex3d(x0+Vrui::Scalar(ovl.ovl_x+ovl.ovl_w)*xs,screenCenter[1]-0.1,z1-Vrui::Scalar(ovl.ovl_y)*ys);
			glTexCoord2i(0,0);
			glVertex3d(x0+Vrui::Scalar(ovl.ovl_x)*xs,screenCenter[1]-0.1,z1-Vrui::Scalar(ovl.ovl_y)*ys);
			glEnd();
			}
		dataItem->overlayTextureVersion=overlaySetVersion;
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		
		glPopAttrib();
		}
	
	glPopMatrix();
	}

void VruiXine::resetNavigation(void)
	{
		/* Calculate the diagonal size of the current display screen: */
		Vrui::Scalar screenSize=Math::sqrt(Math::sqr(screenHeight)+Math::sqr(screenHeight*aspectRatio));
		
		/* Center the current display screen in the display: */
		Vrui::setNavigationTransformation(screenCenter,Math::div2(screenSize),Vrui::Vector(0,0,1));
	}

void VruiXine::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if this is a button press event: */
	if(cbData->newButtonState)
		{
		/* Send an appropriate event to the xine library: */
		xineSendEvent(int(eventId));
		}
	}

void VruiXine::initContext(GLContextData& contextData, GLObject const * glObj) const
	{
	/* Create a context data item and store it in the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(glObj,dataItem);
	
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
	
	/* Initialize the overlay set textures: */
	for(int i=0;i<XINE_VORAW_MAX_OVL;++i)
		{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,dataItem->overlayTextureIds[i]);
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
