#ifndef GRAPPLEMAP_XINE_HPP
#define GRAPPLEMAP_XINE_HPP

#include <xine.h>
#include <xine/xineutils.h>
#include <string>
#include <stdexcept>
#include <memory>

namespace GrappleMap
{
	namespace Xine
	{
		using EnginePtr =
			std::unique_ptr<xine_t, decltype(&xine_exit)>;

		struct Engine : EnginePtr
		{
			Engine(): EnginePtr(xine_new(), &xine_exit)
			{
				if (!get()) throw std::runtime_error("cannot create xine engine");

				xine_config_load(get(), (xine_get_homedir() + std::string("/.xine/config")).c_str());
				xine_init(get());
			}
		};

		using AudioPortPtr =
			std::unique_ptr<xine_audio_port_t, std::function<void(xine_audio_port_t *)>>;

		struct AudioPort: AudioPortPtr
		{
			static void close(Engine const & e, xine_audio_port_t * p)
			{
				xine_close_audio_driver(&*e, p);

			} // can be removed when we can assume C++14

			explicit AudioPort(Engine const & e)
				: AudioPortPtr(
					xine_open_audio_driver(&*e, nullptr, nullptr),
					[&e](xine_audio_port_t * p){ close(e, p); })
			{
				if (!get()) throw std::runtime_error("cannot create xine audio port");
			}
		};

		using VideoPortPtr =
			std::unique_ptr<xine_video_port_t, std::function<void(xine_video_port_t *)>>;

		struct VideoPort: VideoPortPtr
		{
			static void close(Engine const & e, xine_video_port_t * p)
			{
				xine_close_video_driver(&*e, p);
			} // can be removed when we can assume C++14

			VideoPort(Engine const & e, xine_video_port_t * p)
				: VideoPortPtr(p, [&e](xine_video_port_t * q){ close(e, q); })
			{
				if (!p) throw std::runtime_error("cannot create xine video port");
			}

			VideoPort(Engine const & e, raw_visual_t & r)
				: VideoPort(e, xine_open_video_driver(&*e, nullptr, XINE_VISUAL_TYPE_RAW, &r))
			{}
		};

		using StreamPtr = std::unique_ptr<xine_stream_t, decltype(&xine_dispose)>;

		struct Stream: StreamPtr
		{
			Stream(Engine const & e, xine_audio_port_t * audio, VideoPort const & video)
				: StreamPtr(xine_stream_new(&*e, audio, &*video), &xine_dispose)
			{
				if (!get()) throw std::runtime_error("cannot create xine stream");
			}
		};

		using EventQueuePtr = std::unique_ptr<xine_event_queue_t, decltype(&xine_event_dispose_queue)>;

		struct EventQueue: EventQueuePtr
		{
			explicit EventQueue(Stream const & s)
				: EventQueuePtr(xine_event_new_queue(&*s), &xine_event_dispose_queue)
			{
				if (!get()) throw std::runtime_error("cannot create xine event queue");
			}
		};
	}
}

#endif
