#ifndef GRAPPLEMAP_IMAGES_HPP
#define GRAPPLEMAP_IMAGES_HPP

#include "graph.hpp"
#include "headings.hpp"
#include "rendering.hpp"
#include <GL/osmesa.h>
#include <Magick++.h>
#include <future>
#include <queue>
#include <thread>
#include <mutex>

template<typename Data>
class concurrent_queue
{
	std::queue<Data> q;
	mutable std::mutex m;
	std::condition_variable nonempty;
	std::condition_variable nonfull;
	bool nothing_new = false;

	bool full() const
	{
		return q.size() > 16;
	}

public:

	void finish()
	{
		std::unique_lock<std::mutex> lock(m);
		nothing_new = true;
		lock.unlock();
		nonempty.notify_all();
	}

	void push(Data data)
	{
		assert(!nothing_new);

		std::unique_lock<std::mutex> lock(m);
		while (full()) nonfull.wait(lock);

		q.push(std::move(data));

		lock.unlock();
		nonempty.notify_one();
	}

	bool pop(Data & out) // returns false if queue exhausted
	{
		std::unique_lock<std::mutex> lock(m);

		while (q.empty())
		{
			if (nothing_new) return false;

			nonempty.wait(lock);
		}

		out = std::move(q.front());
		q.pop();

		lock.unlock();
		nonfull.notify_one();
		return true;
	}
};


template<typename Job>
class ThreadPool
{
	concurrent_queue<Job> jobs;
	std::vector<std::thread> workers;

	void work()
	{
		Job job;
		while (jobs.pop(job)) process(job);
	}

	public:

		explicit ThreadPool(size_t const n)
		{
			for (size_t i = 0; i != n; ++i)
				workers.emplace_back([this]{ work(); });
		}

		~ThreadPool()
		{
			jobs.finish();
			foreach (w : workers) w.join();
		}

		ThreadPool(ThreadPool const &) = delete;

		void add_job(Job j) { jobs.push(std::move(j)); }
};

namespace GrappleMap {

struct GifGenerationJob
{
	string path;
	vector<Magick::Image> frames;
};

void process(GifGenerationJob &);

struct OSMesaContextPtr
{
	OSMesaContext context = nullptr;

	explicit OSMesaContextPtr(OSMesaContext c): context(c) {}

	~OSMesaContextPtr()
	{
		if (context) OSMesaDestroyContext(context);
	}
};

class ImageMaker
{
	Graph const & graph;
	OSMesaContextPtr ctx;

	void png(
		Position pos, double angle, double ymax, string filename,
		unsigned width, unsigned height, V3 bg_color);

	vector<Magick::Image> make_frames(
		vector<pair<Position, Camera>> const &,
		size_t delay,
		unsigned width, unsigned height,
		V3 bg_color,
		View const &);

	ThreadPool<GifGenerationJob> gif_generators;

	void add_job(string const & path, std::function<vector<Magick::Image>()> make_frames);

public:

	ImageMaker(Graph const &);

	ImageMaker(ImageMaker const &) = delete;
	ImageMaker & operator=(ImageMaker const &) = delete;

	enum BgColor { RedBg, BlueBg, WhiteBg };

	static V3 color(BgColor const c)
	{
		switch (c)
		{
			case RedBg: return V3{1,.878,.878};
			case BlueBg: return V3{.878,.878,1};
			case WhiteBg: return V3{1,1,1};
			default: abort();
		}
	}

	static string css(BgColor const c)
	{
		switch (c)
		{
			case RedBg: return "background:#ffe0e0";
			case BlueBg: return "background:#e0e0ff";
			case WhiteBg: return "";
			default: abort();
		}
	}

	void png(
		Position,
		Camera const &,
		unsigned width, unsigned height,
		string path, V3 bg_color,
		vector<View> const &, unsigned grid_size = 2, unsigned grid_line_width = 2);

	void png(
		pair<Position, Camera> const * pos_beg,
		pair<Position, Camera> const * pos_end,
		unsigned width, unsigned height,
		string path, V3 bg_color,
		vector<View> const &, unsigned grid_size = 2, unsigned grid_line_width = 2);

	string png(
		string output_dir, Position, double ymax, ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string rotation_gif(
		string output_dir, Position, ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string gif(
		string output_dir,
		vector<Position> const & frames,
		ImageView,
		unsigned width, unsigned height, BgColor,
		string base_linkname);

	string gifs(
		string output_dir,
		vector<Position> const & frames,
		unsigned width, unsigned height, BgColor,
		string base_linkname);
};

}

#endif
