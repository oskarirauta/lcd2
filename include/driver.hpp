#pragma once

#include <stdexcept>
#include <utility>
#include <string>
#include <vector>
#include <map>

#include "rgb.hpp"
#include "rect.hpp"
#include "orientation.hpp"
#include "layout.hpp"

namespace drv {

	class DRIVER {

		private:

		protected:

			int _pwidth;
			int _pheight;
			int _backlight;
			virtual RGBA blend(int x, int y);
			std::vector<RGBA> canvas;

			// One-shot: forces the next full-screen blit to write and send
			// every pixel, bypassing the per-pixel dirty/delta check. Set at
			// construction so the first frame fully repaints the panel rather
			// than assuming it starts in the driver's blank canvas state (the
			// panel may retain previous/demo content from before lcd2 ran).
			bool _force_full = true;

		public:

			virtual const std::string name() = 0;
			virtual const std::string device() = 0;
			virtual RGBA rgb(int x, int y);
			virtual int backlight();

			virtual const int BPP() = 0;
			virtual void blit(int x, int y, int width, int height) = 0;
			virtual void blit_fullscreen();
			virtual void blit(const std::vector<RECT>& rects);
			virtual void clear() = 0;
			virtual void backlight(int value) = 0;

			virtual void blit();
			virtual void refresh(); // Alias to blit
			virtual void refresh(const std::vector<RECT>& rects); // Alias to blit(vector..)
			virtual void reset_canvas();

			int pwidth();
			int pheight();

			DRIVER() {}
			virtual ~DRIVER() {}
	};

	extern std::vector<std::string> list;

}
