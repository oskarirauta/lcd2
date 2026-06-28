#include <cstring>
#include <stdexcept>
#include <fstream>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libdrm/drm_mode.h>

#include "throws.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "display.hpp"
#include "rgb.hpp"
#include "driver.hpp"
#include "drivers/drm.hpp"

static expr::VARIABLE fn_drm_brightness(const expr::FUNCTION_ARGS& args);

drv::DRM::DRM(const std::string& device, int backlight, const std::string& backlight_path, int& width, int& height) {

    _dev = device.empty() ? "/dev/dri/card0" : device;
    _backlight = backlight;

    try {
        open_device();
        find_connector();
        create_framebuffer();
        set_crtc();
    } catch (const std::runtime_error& e) {
        destroy_framebuffer();
        if (_saved_crtc) { drmModeFreeCrtc(_saved_crtc); _saved_crtc = nullptr; }
        if (_fd >= 0) { close(_fd); _fd = -1; }
        throws << e.what() << std::endl;
    }

    width = _pwidth;
    height = _pheight;

    this->canvas.resize(_pwidth * _pheight, RGBA(RGBA::BLACK));
    memset(_buffer.map, 0, _buffer.size);

    find_backlight_path(backlight_path);
    this->backlight(backlight);

    CONFIG::functions.append({"brightness", fn_drm_brightness});
    CONFIG::functions.append({"backlight",  fn_drm_brightness});
}

drv::DRM::~DRM() {

    CONFIG::functions.erase("brightness");
    CONFIG::functions.erase("backlight");

    if (_saved_crtc && _fd >= 0) {
        drmModeSetCrtc(_fd, _saved_crtc->crtc_id, _saved_crtc->buffer_id,
                       _saved_crtc->x, _saved_crtc->y,
                       &_connector_id, 1, &_saved_crtc->mode);
        drmModeFreeCrtc(_saved_crtc);
        _saved_crtc = nullptr;
    }

    destroy_framebuffer();

    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }

    logger::verbose["driver"] << "drm: driver exiting" << std::endl;
}

const int drv::DRM::BPP() {
    return 4; // XRGB8888
}

void drv::DRM::open_device() {

    _fd = open(_dev.c_str(), O_RDWR | O_CLOEXEC);
    if (_fd < 0)
        throw std::runtime_error("drm: failed to open " + _dev + ": " + strerror(errno));

    uint64_t has_dumb = 0;
    if (drmGetCap(_fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb)
        throw std::runtime_error("drm: device " + _dev + " does not support dumb buffers");

    logger::info["driver"] << "drm: opened " << _dev << std::endl;
}

void drv::DRM::find_connector() {

    drmModeRes* res = drmModeGetResources(_fd);
    if (!res)
        throw std::runtime_error("drm: failed to get DRM resources");

    drmModeConnector* conn = nullptr;

    for (int i = 0; i < res->count_connectors && !conn; i++) {
        drmModeConnector* c = drmModeGetConnector(_fd, res->connectors[i]);
        if (!c) continue;
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            conn = c;
            _connector_id = c->connector_id;
        } else {
            drmModeFreeConnector(c);
        }
    }

    if (!conn) {
        drmModeFreeResources(res);
        throw std::runtime_error("drm: no connected display found on " + _dev);
    }

    // Use first (preferred/highest resolution) mode
    _mode    = conn->modes[0];
    _pwidth  = _mode.hdisplay;
    _pheight = _mode.vdisplay;

    logger::info["driver"] << "drm: mode " << _pwidth << "x" << _pheight
                           << " @" << _mode.vrefresh << "Hz" << std::endl;

    // Find CRTC via current encoder, or search for a compatible one
    drmModeEncoder* enc = nullptr;

    if (conn->encoder_id)
        enc = drmModeGetEncoder(_fd, conn->encoder_id);

    if (enc && enc->crtc_id) {
        _crtc_id = enc->crtc_id;
        _saved_crtc = drmModeGetCrtc(_fd, _crtc_id);
        drmModeFreeEncoder(enc);
    } else {
        if (enc) { drmModeFreeEncoder(enc); enc = nullptr; }

        for (int i = 0; i < conn->count_encoders && !_crtc_id; i++) {
            drmModeEncoder* e = drmModeGetEncoder(_fd, conn->encoders[i]);
            if (!e) continue;
            for (int j = 0; j < res->count_crtcs && !_crtc_id; j++) {
                if (e->possible_crtcs & (1 << j)) {
                    _crtc_id = res->crtcs[j];
                    _saved_crtc = drmModeGetCrtc(_fd, _crtc_id);
                }
            }
            drmModeFreeEncoder(e);
        }
    }

    drmModeFreeConnector(conn);
    drmModeFreeResources(res);

    if (!_crtc_id)
        throw std::runtime_error("drm: no suitable CRTC found");

    logger::info["driver"] << "drm: connector " << _connector_id
                           << " CRTC " << _crtc_id << std::endl;
}

void drv::DRM::create_framebuffer() {

    struct drm_mode_create_dumb cd{};
    cd.width  = _pwidth;
    cd.height = _pheight;
    cd.bpp    = 32;

    if (drmIoctl(_fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd) < 0)
        throw std::runtime_error(std::string("drm: create dumb buffer failed: ") + strerror(errno));

    _buffer.handle = cd.handle;
    _buffer.stride = cd.pitch;
    _buffer.size   = cd.size;

    if (drmModeAddFB(_fd, _pwidth, _pheight, 24, 32,
                     _buffer.stride, _buffer.handle, &_buffer.fb_id) < 0) {
        destroy_framebuffer();
        throw std::runtime_error("drm: failed to add framebuffer");
    }

    struct drm_mode_map_dumb md{};
    md.handle = _buffer.handle;

    if (drmIoctl(_fd, DRM_IOCTL_MODE_MAP_DUMB, &md) < 0) {
        destroy_framebuffer();
        throw std::runtime_error(std::string("drm: map dumb buffer failed: ") + strerror(errno));
    }

    void* ptr = mmap(nullptr, _buffer.size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, _fd, static_cast<off_t>(md.offset));
    if (ptr == MAP_FAILED) {
        destroy_framebuffer();
        throw std::runtime_error(std::string("drm: mmap failed: ") + strerror(errno));
    }

    _buffer.map = static_cast<uint8_t*>(ptr);

    logger::info["driver"] << "drm: framebuffer ready, stride=" << _buffer.stride << std::endl;
}

void drv::DRM::set_crtc() {

    if (drmModeSetCrtc(_fd, _crtc_id, _buffer.fb_id, 0, 0,
                       &_connector_id, 1, &_mode) < 0)
        throw std::runtime_error(std::string("drm: set CRTC failed: ") + strerror(errno));

    logger::info["driver"] << "drm: CRTC active" << std::endl;
}

void drv::DRM::destroy_framebuffer() {

    if (_buffer.map) {
        munmap(_buffer.map, _buffer.size);
        _buffer.map = nullptr;
    }

    if (_buffer.fb_id && _fd >= 0) {
        drmModeRmFB(_fd, _buffer.fb_id);
        _buffer.fb_id = 0;
    }

    if (_buffer.handle && _fd >= 0) {
        struct drm_mode_destroy_dumb dd{};
        dd.handle = _buffer.handle;
        drmIoctl(_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
        _buffer.handle = 0;
    }
}

void drv::DRM::find_backlight_path(const std::string& configured_path) {

    if (configured_path == "disabled") {
        _backlight_disabled = true;
        logger::info["driver"] << "drm: backlight disabled" << std::endl;
        return;
    }

    if (!configured_path.empty() && configured_path != "auto") {
        _backlight_path = configured_path;
        std::ifstream f(_backlight_path + "/max_brightness");
        if (f) f >> _backlight_max;
        logger::info["driver"] << "drm: backlight at " << _backlight_path
                               << " (max=" << _backlight_max << ")" << std::endl;
        return;
    }

    namespace fs = std::filesystem;
    const std::string bl_dir = "/sys/class/backlight";

    if (!fs::exists(bl_dir)) return;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(bl_dir, ec)) {

        _backlight_path = entry.path().string();
        std::ifstream f(_backlight_path + "/max_brightness");
        if (f) f >> _backlight_max;

        logger::info["driver"] << "drm: backlight at " << _backlight_path
                               << " (max=" << _backlight_max << ")" << std::endl;
        break;
    }
}

void drv::DRM::write_pixel(int x, int y, const RGBA& c) {

    if (!_buffer.map || x < 0 || x >= _pwidth || y < 0 || y >= _pheight)
        return;

    uint8_t* p = _buffer.map + static_cast<ptrdiff_t>(y) * _buffer.stride + x * 4;
    p[0] = c.B;
    p[1] = c.G;
    p[2] = c.R;
    p[3] = 0; // unused (XRGB8888)
}

void drv::DRM::mark_dirty() {

    // Notify the kernel driver that framebuffer content has changed.
    // Required for USB-attached DRM displays (ax206, UDL, etc.) that don't
    // scan the dumb-buffer automatically — they need an explicit upload trigger.
    // Pass an explicit full-screen clip: some drivers ignore nullptr/0-count clips
    // entirely and only trigger an upload when at least one clip rect is provided.
    // Standard GPU drivers ignore or silently reject this ioctl, so errors are fine.
    // NOTE: Do NOT fall back to drmModePageFlip — it waits for vblank and blocks
    // the render thread for the full refresh period on USB displays (ax206 etc).
    drmModeClip clip{ 0, 0, static_cast<uint16_t>(_pwidth), static_cast<uint16_t>(_pheight) };
    drmModeDirtyFB(_fd, _buffer.fb_id, &clip, 1);
}

// Collect layer vectors for the current page once, avoiding per-pixel map lookups.
// Returns false if the page has no renderable layers.
static bool collect_layers(int page_no,
    std::vector<std::pair<int, const std::vector<RGBA>*>>& out)
{
    auto pit = display->canvas.find(page_no);
    if (pit == display->canvas.end() || pit->second.empty())
        return false;
    out.clear();
    for (auto& [l, v] : pit->second)
        out.emplace_back(l, &v);
    return true;
}

// Blend pixel at canvas index idx using pre-collected layer references.
static RGBA blend_pixel(
    const std::vector<std::pair<int, const std::vector<RGBA>*>>& layers,
    int idx)
{
    RGBA ret(RGBA::BL.R, RGBA::BL.G, RGBA::BL.B, 0x00);
    int o = -1;

    for (auto& [l, v] : layers) {
        if (idx < (int)v->size() && (*v)[idx].A == 0xff) { o = l; break; }
    }

    for (auto& [l, v] : layers) {
        if (l < o || idx >= (int)v->size()) continue;
        const RGBA& p = (*v)[idx];
        switch (p.A) {
            case 0: break;
            case 0xff:
                ret = RGBA(p.R, p.G, p.B, 0xff);
                break;
            default: {
                unsigned int R = (p.R * p.A + ret.R * (0xff - p.A)) / 0xff;
                unsigned int G = (p.G * p.A + ret.G * (0xff - p.A)) / 0xff;
                unsigned int B = (p.B * p.A + ret.B * (0xff - p.A)) / 0xff;
                ret = RGBA((unsigned char)R, (unsigned char)G, (unsigned char)B, 0xff);
            }
        }
    }

    return ret;
}

void drv::DRM::blit(int x, int y, int width, int height) {

    if (!_buffer.map) return;

    std::vector<std::pair<int, const std::vector<RGBA>*>> layers;
    if (!collect_layers(display->page_number(), layers)) return;

    bool any_written = false;
    for (int _y = y; _y < y + height && _y < _pheight; _y++) {
        for (int _x = x; _x < x + width && _x < _pwidth; _x++) {
            int idx = _y * _pwidth + _x;
            RGBA c = blend_pixel(layers, idx);
            if (this->canvas[idx] != c) {
                this->canvas[idx] = c;
                write_pixel(_x, _y, c);
                any_written = true;
            }
        }
    }

    if (any_written)
        mark_dirty();
}

void drv::DRM::blit_fullscreen() {

    if (!_buffer.map) return;

    std::vector<std::pair<int, const std::vector<RGBA>*>> layers;
    if (!collect_layers(display->page_number(), layers)) return;

    bool force = _force_full;
    _force_full = false;

    bool any_written = false;
    for (int y = 0; y < _pheight; y++) {
        for (int x = 0; x < _pwidth; x++) {
            int idx = y * _pwidth + x;
            RGBA c = blend_pixel(layers, idx);
            if (force || this->canvas[idx] != c) {
                this->canvas[idx] = c;
                write_pixel(x, y, c);
                any_written = true;
            }
        }
    }

    if (any_written || force)
        mark_dirty();
}

void drv::DRM::clear() {

    if (!_buffer.map) return;

    // Two-step blank for delta-based remote drivers. USB-attached DRM panels
    // (ax206, UDL, ...) often upload only pixels that differ from a kernel-side
    // shadow buffer that starts all-black. Writing pure black over that black
    // shadow is a no-op there, so whatever the panel already shows (e.g. the
    // power-on factory demo image) survives. Push a near-black sentinel
    // (#000001, indistinguishable to the eye) across the whole screen first so
    // a real change is forced up to the panel, then overwrite it with black:
    // black now differs from the sentinel and is uploaded too, leaving the
    // panel truly black. (The kernel's own coalescing usually merges the two
    // into a single black frame, so no flash is visible.)
    RGBA sentinel(0, 0, 1, 255); // #000001
    std::fill(this->canvas.begin(), this->canvas.end(), sentinel);
    for (int y = 0; y < _pheight; y++)
        for (int x = 0; x < _pwidth; x++)
            write_pixel(x, y, sentinel);
    mark_dirty();

    std::fill(this->canvas.begin(), this->canvas.end(), RGBA(RGBA::BLACK));
    memset(_buffer.map, 0, _buffer.size);
    mark_dirty();
}

void drv::DRM::backlight(int value) {

    if (_backlight_disabled) return;

    if (value < 0) value = 0;
    else if (value > 10) value = 10;

    _backlight = value;

    if (_backlight_path.empty()) return;

    int scaled = (value * _backlight_max) / 10;
    std::ofstream f(_backlight_path + "/brightness");
    if (f) f << scaled;
}

static expr::VARIABLE fn_drm_brightness(const expr::FUNCTION_ARGS& args) {

    if (!display || !display->driver) {
        logger::error["function"] << "backlight: driver not available" << std::endl;
        return (double)0;
    }

    if (args.empty())
        return (double)display->driver->backlight();

    if (!args[0].number_convertible().empty()) {
        logger::error["function"] << "backlight: argument must be a number" << std::endl;
        return (double)display->driver->backlight();
    }

    display->driver->backlight(args[0].to_int());
    return (double)display->driver->backlight();
}
