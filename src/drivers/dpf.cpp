#include <cstring>
#include <vector>

#include "throws.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "display.hpp"
#include "orientation.hpp"
#include "rect.hpp"
#include "driver.hpp"
#include "drivers/dpf.hpp"

static const int	DPF_BPP = 2;
static const short	AX206_VID = 0x1908; // AX206 USB Vendor ID
static const short	AX206_PID = 0x0102; // AX206 USB Product ID

// forward declaration
static expr::VARIABLE fn_brightness(const expr::FUNCTION_ARGS& args);

drv::DPF::DPF(const std::string& device, int backlight, int& width, int& height) {

	this -> _dev = device;
	this -> _backlight = backlight;

	try {
		this -> ax_open();

	} catch ( const std::runtime_error& e ) {

		throws << e.what() << std::endl;
	}

	this -> ax_backlight(0);
	this -> canvas.resize(this -> _pwidth * this -> _pheight, RGBA(RGBA::BLACK));
	this -> rect_init(this -> bounds);

	width = this -> _pwidth;
	height = this -> _pheight;

	this -> fill(RGBA(RGBA::BLACK));
	this -> ax_backlight(backlight);
	this -> reset_canvas();

	CONFIG::functions.append({"brightness", fn_brightness });
	CONFIG::functions.append({"backlight", fn_brightness });
}

drv::DPF::~DPF() {

	if ( display == nullptr || !display -> goodbye()) {

		this -> ax_backlight(0);
		this -> fill(RGBA(RGBA::BLACK));

	}

	CONFIG::functions.erase("brightness");
	CONFIG::functions.erase("backlight");

	logger::verbose["driver"] << "dpf_ax: driver exiting" << std::endl;
	this -> ax_close();
}

const int drv::DPF::BPP() {

	return DPF_BPP;
}

void drv::DPF::rect_init(RECT& rect) {

	rect.min = RECT::POINT(0, 0);
	rect.max = RECT::POINT(this -> _pwidth - 1, this -> _pheight - 1);
}

void drv::DPF::rect_reset(RECT& rect) {

	rect.min = RECT::POINT(this -> _pwidth - 1, this -> _pheight - 1);
	rect.max = RECT::POINT(0, 0);
}

std::vector<unsigned char> drv::DPF::rect_args(const RECT& rect) {

	return {
		static_cast<unsigned char>(drv::DPF::USBCMD::BLIT),
		static_cast<unsigned char>(rect.min.x),
		static_cast<unsigned char>(rect.min.x >> 8),
		static_cast<unsigned char>(rect.min.y),
		static_cast<unsigned char>(rect.min.y >> 8),
		static_cast<unsigned char>(rect.max.x - 1),
		static_cast<unsigned char>((rect.max.x - 1) >> 8),
		static_cast<unsigned char>(rect.max.y - 1),
		static_cast<unsigned char>((rect.max.y - 1) >> 8),
		0x00 };
}

void drv::DPF::set_pixel(int x, int y, const RGBA& c) {

	bool changed = false;

	if ( x < 0 || x >= this -> _pwidth || y < 0 || y >= this -> _pheight ) {

		logger::warning["driver"] << "dpf: x/y out of bounds" <<
			logger::detail("x=" + std::to_string(x) + ", y=" + std::to_string(y) +
					", max x=" + std::to_string(this -> _pwidth) + ", max y=" + std::to_string(this -> _pheight)) << std::endl;
		return;
	}

	if ( this -> canvas[(y * this -> _pwidth ) + x] != c ) {
		this -> canvas[(y * this -> _pwidth ) + x ] = c;
		changed = true;
	}

	if ( changed ) {
		if ( x < this -> bounds.min.x ) this -> bounds.min.x = x;
		if ( x > this -> bounds.max.x ) this -> bounds.max.x = x;
		if ( y < this -> bounds.min.y ) this -> bounds.min.y = y;
		if ( y > this -> bounds.max.y ) this -> bounds.max.y = y;
	}
}

void drv::DPF::fill(const RGBA& c) {

	std::fill(this -> canvas.begin(), this -> canvas.end(), c);
	this -> ax_blit(this -> canvas, RECT(0, 0, this -> _pwidth + 1, this -> _pheight + 1));

	// Reset dirty rectangle
	this -> rect_reset(this -> bounds);
}

void drv::DPF::backlight(int value) {

	if ( value < 0 || value > 7 )
		logger::warning["driver"] << "dpf_ax: backlight setting out of range 0-7" << std::endl;

	this -> ax_backlight(value < 0 ? 0 : ( value > 7 ? 7 : value ));
}

void drv::DPF::blit(int x, int y, int width, int height) {

	this -> rect_reset(this -> bounds);

	for ( int _y = y; _y < y + height; _y++ ) {
		for ( int _x = x; _x < x + width; _x++ ) {
			this -> set_pixel(_x, _y, this -> blend(_x, _y));
		}
	}

	if ( this -> bounds.min.x > this -> bounds.max.x ||
		this -> bounds.min.y > this -> bounds.max.y )
		return;

	std::vector<RGBA> xferBuf;
	for ( int _y = this -> bounds.min.y; _y <= this -> bounds.max.y; _y++ )
		for ( int _x = this -> bounds.min.x; _x <= this -> bounds.max.x; _x++ )
			xferBuf.push_back(this -> canvas[(_y * ( this -> _pwidth )) + _x]);

	// Send the buffer
	this -> ax_blit(xferBuf, RECT(this -> bounds.min.x, this -> bounds.min.y,
					this -> bounds.max.x + 1, this -> bounds.max.y + 1));

	// Reset dirty rectangle
	this -> rect_reset(this -> bounds);
}

void drv::DPF::blit_fullscreen() {

	int y = 0, _y;
	RECT rect;
	std::vector<RECT> rects;

	while ( true ) {

		if ( y > this -> _pheight )
			break;

		bool changed = false;
		this -> rect_reset(rect);

		// find first line that has changed
		for ( _y = y; _y < this -> _pheight && !changed; _y++ ) {
			for ( int _x = 0; _x < this -> _pwidth && !changed; _x++ ) {
				if ( auto c = this -> blend(_x, _y); this -> canvas[(_y * this -> _pwidth ) + _x] != c ) {

					//this -> canvas[(_y * this -> _pwidth ) + _x ] = c;
					rect.min = RECT::POINT(_x, _y);
					rect.max = RECT::POINT(_x, _y);
					changed = true;
				}
			}
		}

		if ( !changed )
			break;

		y = _y ;

		// find rest of changed pixels
		for ( _y = y; _y < this -> _pheight && changed; _y++ ) {

			changed = false;

			for ( int _x = 0; _x < this -> _pwidth; _x++ ) {

				if ( auto c = this -> blend(_x, _y); this -> canvas[(_y * this -> _pwidth ) + _x] != c ) {

					if ( _x < rect.min.x ) rect.min.x = _x;
					if ( _x > rect.max.x ) rect.max.x = _x;
					if ( _y > rect.max.y ) rect.max.y = _y;
					changed = true;
				}
			}
		}

		if ( changed )
			break;

		// small hack to avoid lost pixels
		for ( int t = 0; t < 5 && rect.max.x < this -> _pwidth; t++, rect.max.x++);
		for ( int t = 0; t < 5 && rect.max.y > this -> _pheight; t++, rect.max.y++);
		for ( int t = 0; t < 5 && rect.min.x > 0; t++, rect.min.x--);
		for ( int t = 0; t < 5 && rect.min.y > 0; t++, rect.min.y--);

		for ( _y = rect.min.y; _y <= rect.max.y; _y++ )
			for ( int _x = rect.min.x; _x <= rect.max.x; _x++ )
				if ( auto c = this -> blend(_x, _y); this -> canvas[(_y * this -> _pwidth ) + _x] != c )
					this -> canvas[(_y * this -> _pwidth ) + _x ] = c;

		rects.push_back(rect);
	}

	if ( rects.empty())
		return;

	for ( const auto& bounds : rects ) {

		std::vector<RGBA> xferBuf;

		for ( _y = bounds.min.y; _y <= bounds.max.y && _y <= this -> _pheight; _y++ )
			for ( int _x = bounds.min.x; _x <= bounds.max.x; _x++ )
				xferBuf.push_back(this -> canvas[(_y * ( this -> _pwidth )) + _x]);

		// Send the buffer
		this -> ax_blit(xferBuf, RECT(bounds.min.x, bounds.min.y,
                                        bounds.max.x + 1, bounds.max.y + 1));

		// Reset dirty rectangle - though, we propably didn't touch it..
		this -> rect_reset(this -> bounds);
	}

}

void drv::DPF::clear() {

	this -> blit(0, 0, this -> _pwidth, this -> _pheight);
}

std::vector<unsigned char> g_excmd = {
	0xcd, 0, 0, 0,
	0, 6, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

static std::vector<unsigned char> g_buf = {	// completed size is always 31
	0x55, 0x53, 0x42, 0x43,			// dCBWSignature
	0xde, 0xad, 0xbe, 0xef,			// dCBWTag
	0x00, 0x80, 0x00, 0x00,			// dCBWLength
	0x00,					// bmCBWFlags: 0x80: data in (dev to host), 0x00: Data out
	0x00, 16 };				//cmd size is always 16

void drv::DPF::ax_open() {

	int index = -1;
	usb_dev_handle *u;

	if ( this -> _dev.size() == 4 && ( common::has_prefix(this -> _dev, "usb") || common::has_prefix(this -> _dev, "dpf"))) {

		std::string dev = this -> _dev;
		dev.erase(0, 3);

		try {
			index = std::stoi(dev);
		} catch ( std::runtime_error& e ) {
			throws << "dpf_ax_open: cannot parse device index number from " << this -> _dev <<
					", example value could be for example usb0" << std::endl;
		}
	}

	if ( index < 0 || index > 9 )
		throws << "dpf_ax_open: invalid device " << this -> _dev << ", please specify a string like 'usb0'" << std::endl;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	struct usb_bus *b = usb_get_busses();
	struct usb_device *d = nullptr;
	int enumeration = 0;
	bool found = false;

	while ( b && !found ) {

		d = b -> devices;
		while ( d ) {

			if (( d -> descriptor.idVendor == AX206_VID ) &&
				( d -> descriptor.idProduct == AX206_PID )) {

					logger::info["driver"] << "dpf_ax_open: found AX206 #" << enumeration + 1 << std::endl;

					if ( enumeration == index ) {
						found = true;
						break;
					} else enumeration++;
			}
			d = d -> next;
		}
		b = b -> next;
	}

	if ( !d )
		throws << "dpf_ax_open: no matching USB device " << this -> _dev << " found" << std::endl;

	if ( u = usb_open(d); u == nullptr )
		throws << "dpf_ax_open: failed to open usb device " << this -> _dev << std::endl;

	if ( usb_claim_interface(u, 0) < 0 ) {

		usb_close(u);
		throws << "dpf_ax_open: failed to claim usb device" << std::endl;
	}

	this -> udev = u;

	std::vector<unsigned char> buf(5);
	std::vector<unsigned char> cmd(g_excmd);
	cmd[5] = 2; // get LCD parameters

	if ( this -> wrap_scsi(cmd, IN, &buf) == 0 ) {

		this -> _pwidth = (buf[0]) | (buf[1] << 8);
		this -> _pheight = (buf[2]) | (buf[3] << 8);
		logger::info["driver"] << "dpf_ax_open: got LCD dimensions: " <<
			this -> _pwidth << "x" << this -> _pheight << std::endl;
	} else {

		this -> ax_close();
		throws << "dpf_ax_open: error reading LCD dimensions" << std::endl;
	}
}

void drv::DPF::ax_close() {

	usb_release_interface(this -> udev, 0);
	usb_close(this -> udev);
	this -> udev = nullptr;
}

void drv::DPF::ax_blit(std::vector<RGBA>& buf, const RECT& rect) {

	if ( buf.empty())
		return;

	std::vector<unsigned char> xferBuf;
	std::for_each(buf.begin(), buf.end(), [&xferBuf](RGBA c) {
		std::vector<unsigned char>rgb = c.RGB565();
		std::copy(rgb.begin(), rgb.end(), std::back_inserter(xferBuf));
	});

	std::vector<unsigned char> cmd(g_excmd);
	std::vector<unsigned char> args(this -> rect_args(rect));
	std::copy(args.begin(), args.end(), cmd.begin() + 6);

	this -> wrap_scsi(cmd, OUT, &xferBuf);
}

void drv::DPF::ax_backlight(int value) {

	if ( value < 0 ) value = 0;
	else if ( value > 7 ) value = 7;

	this -> _backlight = value;

	std::vector<unsigned char> cmd(g_excmd);
	std::vector<unsigned char> args = {
		static_cast<unsigned char>(USBCMD::SETPROPERTY), 0x01, 0x00,
		static_cast<unsigned char>(value),
		static_cast<unsigned char>(value >> 8)
	};

	std::copy(args.begin(), args.end(), cmd.begin() + 6);
	this -> wrap_scsi(cmd, OUT, nullptr);
}

int drv::DPF::wrap_scsi(const std::vector<unsigned char>& cmd, const DIR& dir, std::vector<unsigned char>* data) {

	size_t block_len = data == nullptr ? 0 : data -> size();

	std::vector<unsigned char> ansbuf(13);
	std::vector<unsigned char> buf(g_buf); // completed buf size = 31
	std::vector<unsigned char> vec_len = {
		static_cast<unsigned char>(block_len),
		static_cast<unsigned char>(block_len >> 8),
		static_cast<unsigned char>(block_len >> 16),
		static_cast<unsigned char>(block_len >> 24)
	};

	std::copy(cmd.begin(), cmd.end(), std::back_inserter(buf));
	std::copy(vec_len.begin(), vec_len.end(), buf.begin() + 8);

	if ( int ret = usb_bulk_write(this -> udev, static_cast<unsigned char>(DIR::OUT), reinterpret_cast<char*>(buf.data()), buf.size(), 1000); ret < 0 )
		return ret;

	if ( dir == OUT && data != nullptr ) {

		if ( int ret = usb_bulk_write(this -> udev, static_cast<unsigned char>(dir), reinterpret_cast<char*>(data -> data()), data -> size(), 1000); ret < 0 ) {

			logger::error["driver"] << "dpf_ax: bulk write" << std::endl;
			return ret;
		}

	} else if ( dir == IN && data != nullptr ) {

		if ( int ret = usb_bulk_read(this -> udev, static_cast<unsigned char>(dir), reinterpret_cast<char*>(data -> data()), block_len, 4000); ret != (int)block_len ) {

			logger::error["driver"] << "dpf_ax: bulk read, " << ret << " != " << (int)block_len << std::endl;
			return ret;
		}
	}

	// get ACK
	int retry = 0;
	int timeout = 0;

	do {
		timeout = 0;
		if ( int ret = usb_bulk_read(this -> udev, static_cast<unsigned char>(DIR::IN), reinterpret_cast<char*>(ansbuf.data()), ansbuf.size(), 5000);
			ret != (int)ansbuf.size()) {

			logger::error["driver"] << "dpf_ax: bulk ACK read" << std::endl;
			timeout = 1;
		}

		retry++;

	} while ( timeout && retry < 5 );

	if ( std::string(ansbuf.begin(), ansbuf.begin() + 4) != "USBS" ) {

		logger::error["driver"] << "dpf_ax: got invalid reply " << std::string(ansbuf.begin(), ansbuf.begin() + 4) << std::endl;
		return -1;
	}

	return ansbuf[12];
}

static expr::VARIABLE fn_brightness(const expr::FUNCTION_ARGS& args) {

	if ( display == nullptr || display -> driver == nullptr ) {

		logger::error["function"] << "backlight function has failed, driver not available" << std::endl;
		return (double)0;
	}

	if ( args.empty())
		return display -> driver -> backlight();

	else if ( !args[0].number_convertible().empty()) {

		std::string a = args[0].string_convertible().empty() ? ( ", '" + args[0].to_string() + "' is not valid as number" ) : "";
		logger::error["plugin"] << "invalid argument for brightness plugin, argument must be number" << a << std::endl;
		return display -> driver -> backlight();
	}

	int value = args[0].to_int();

	display -> driver -> backlight(value);
	return display -> driver -> backlight();
}
