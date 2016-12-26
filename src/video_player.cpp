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

#include "video_player.hpp"

#include <xine/video_out.h>
#include <boost/regex.hpp>

namespace GrappleMap {

typedef GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3> Vertex; // Type for vertices to render the video display screen

void VideoPlayer::xineEventCallback(void* /*userData*/,const xine_event_t* event)
	{
	
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

void VideoPlayer::xineOutputCallback(void* userData,int frameFormat,int frameWidth,int frameHeight,double frameAspect,void* data0,void* data1,void* data2)
	{
	VideoPlayer* thisPtr=static_cast<VideoPlayer*>(userData);
	
	/* Prepare a new slot in the video frame triple buffer: */
	VideoFrame& newFrame=thisPtr->frame.startNewValue();
	
	/* Re-allocate the frame's storage if the video format has changed: */
	if(newFrame.format!=frameFormat||newFrame.size[0]!=frameWidth||newFrame.size[1]!=frameHeight)
		{
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
	newFrame.aspectRatio=frameAspect;

	/* Post the new video frame and wake up the main thread: */
	thisPtr->frame.postNewValue();
	// TODO Vrui::requestUpdate();
	}

void VideoPlayer::xineOverlayCallback(void * /*userData*/, int /*numOverlays*/, raw_overlay_t *)
{
}

void VideoPlayer::xineSendEvent(int eventId)
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
		event.stream = &*stream;
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

raw_visual_t VideoPlayer::mkRawVisual()
{
	raw_visual_t r;
	r.user_data = this;
	r.supported_formats = XINE_VORAW_YV12 | XINE_VORAW_RGB;
	r.raw_output_cb = xineOutputCallback;
	r.raw_overlay_cb = xineOverlayCallback;
	return r;
}

VideoPlayer::VideoPlayer(Threads::TripleBuffer<VideoFrame> & o, string const filename)
	: frame(o)
{
	xine_event_create_listener_thread(&*eventQueue,xineEventCallback,this);
	
	if(!xine_open(&*stream,filename.c_str())||!xine_play(&*stream,0,0))
		error("Load Video: Unable to play video file: " + filename);
}

VideoFrame makeFrame(xine_video_frame_t const & f)
{
	vo_frame_t * realframe = (vo_frame_t*) f.xine_frame;

	VideoFrame b;
	b.size[0] = f.width;
	b.size[1] = f.height;
	b.aspectRatio = f.aspect_ratio;

	switch (realframe->format)
	{
		case XINE_IMGFMT_YV12:
			b.format = XINE_VORAW_YV12;

			/* Allocate full-size Y plane and half-size U and V planes: */
			b.planes[0].resize(b.size[1]*b.size[0]);
			b.planes[1].resize((b.size[1]>>1)*(b.size[0]>>1));
			b.planes[2].resize((b.size[1]>>1)*(b.size[0]>>1));
			std::copy_n(realframe->base[0], b.size[1]*b.size[0], b.planes[0].data());
			std::copy_n(realframe->base[1], (b.size[1]>>1)*(b.size[0]>>1), b.planes[1].data());
			std::copy_n(realframe->base[2], (b.size[1]>>1)*(b.size[0]>>1), b.planes[2].data());
			break;
		case XINE_IMGFMT_YUY2:
			b.format = XINE_VORAW_YUY2;
			/* Allocate interleaved YU and Y2 plane: */
			b.planes[0] = vector<unsigned char>(realframe->base[0], realframe->base[0] + b.size[1]*b.size[0]*2);
			break;
		case XINE_VORAW_RGB:
			b.format = XINE_VORAW_RGB;
			/* Allocate interleaved RGB plane: */
			b.planes[0] = vector<unsigned char>(realframe->base[0], realframe->base[0] + b.size[1]*b.size[0]*3);
			break;
		default:
			error("unrecognized format");
	}

	return b;
}

TimeInVideo videoTimeFromArg(string const & s)
{
	boost::regex r("(.+)@([0-9]+):([0-9]+):([0-9]+)");

	boost::cmatch m;

	if (!boost::regex_match(s.c_str(), m, r)) return {s, 0};

	int const
		hours = std::stoi(m[2]),
		minutes = std::stoi(m[3]),
		seconds = std::stoi(m[4]),
		millis = ((hours * 60 + minutes) * 60 + seconds) * 1000;

	return {m[1], millis};
}

vector<VideoFrame> loadVideoFrames(TimeInVideo const & tiv)
{
	Xine::Engine xine;
	Xine::VideoPort videoOutPort{xine, xine_new_framegrab_video_port(&*xine)};
	Xine::Stream stream{xine, nullptr /* no audio */, videoOutPort};

	if(!xine_open(&*stream, tiv.mrl.c_str()) || !xine_play(&*stream, 0, tiv.time))
		error("cannot load media: " + tiv.mrl);

	vector<VideoFrame> r;
	xine_video_frame_t fr;
	unsigned c = 200;

	while (xine_get_next_video_frame(&*videoOutPort, &fr) && --c)
	{
		r.push_back(makeFrame(fr));
		xine_free_video_frame(&*videoOutPort, &fr);
	}

	return r;
}

}
