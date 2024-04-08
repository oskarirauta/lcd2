#pragma once

#include <string>
#include <vector>
#include <usb.h>

#include "rgb.hpp"
#include "throws.hpp"
#include "driver.hpp"
#include "orientation.hpp"
#include "rect.hpp"

namespace drv {

	class DPF : public drv::DRIVER {

		private:

			enum DIR : unsigned char {
				IN = 0x01,
				OUT = 0x81
			};

			enum USBCMD : unsigned char {
				SETPROPERTY = 0x01,
				BLIT = 0x12
			};

			std::string _dev;
			usb_dev_handle *udev = nullptr;

			// area needing re-drawing
			RECT bounds;

			void rect_init(RECT& rect);
			void rect_reset(RECT& rect);
			std::vector<unsigned char> rect_args(const RECT& rect);

			void set_pixel(int x, int y, const RGBA& c);
			void fill(const RGBA& c);
			int  wrap_scsi(const std::vector<unsigned char>& cmd, const DIR& dir, std::vector<unsigned char>* data);
			void ax_blit(std::vector<RGBA>& buf, const RECT& rect);
			void ax_backlight(int value);

			void ax_open();
			void ax_close();

		public:

			virtual const std::string name() override { return "DPF"; }
			virtual const std::string device() override { return this -> _dev; }
			virtual const int BPP() override;
			virtual ~DPF() override;

			virtual void backlight(int value) override;
			virtual void blit(int x, int y, int width, int height) override;
			virtual void blit_fullscreen() override;
			virtual void clear() override;

			DPF(const std::string& device, int backlight, int& width, int& height);
	};

}
