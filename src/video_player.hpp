#ifndef GRAPPLEMAP_VIDEO_PLAYER_HPP
#define GRAPPLEMAP_VIDEO_PLAYER_HPP

#define XINE_ENABLE_EXPERIMENTAL_FEATURES

#include "xine_wrap.hpp"
#include "video_monitor.hpp"
#include "util.hpp"

namespace GrappleMap
{
	class VideoPlayer
	{
		Threads::TripleBuffer<VideoFrame> & frame;
		Xine::Engine xine;
		raw_visual_t rawVisual = mkRawVisual();
		Xine::VideoPort videoOutPort{xine, rawVisual};
		Xine::AudioPort audioOutPort{xine};
		Xine::Stream stream{xine, audioOutPort.get(), videoOutPort};
		Xine::EventQueue eventQueue{stream};
		
		raw_visual_t mkRawVisual();
		static void xineEventCallback(void * userData, xine_event_t const *);
		static void xineOutputCallback(void* userData, int frameFormat, int frameWidth, int frameHeight,double frameAspect,void* data0,void* data1,void* data2);
		static void xineOverlayCallback(void* userData, int numOverlays, raw_overlay_t *);
		void xineSendEvent(int eventId);

		public:

			VideoPlayer(Threads::TripleBuffer<VideoFrame> & output, string filename);
	};

	struct TimeInVideo
	{
		string mrl;
		int time; // in milliseconds
	};

	TimeInVideo videoTimeFromArg(string const &);

	vector<VideoFrame> videoFrames(TimeInVideo const &);

	class PrebufferedVideoPlayer
	{
		Threads::TripleBuffer<VideoFrame> & output;
		vector<VideoFrame> frames;
		
		public:

			PrebufferedVideoPlayer(Threads::TripleBuffer<VideoFrame> & output, vector<VideoFrame> frames);

			void seek(unsigned);
	};
}

#endif
