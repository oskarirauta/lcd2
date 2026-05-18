#pragma once

#include <string>
#include <cstdint>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "rgb.hpp"
#include "driver.hpp"
#include "rect.hpp"

namespace drv {

class DRM : public drv::DRIVER {

    private:

        struct DumbBuffer {
            uint32_t handle = 0;
            uint32_t fb_id = 0;
            uint32_t stride = 0;
            uint64_t size = 0;
            uint8_t* map = nullptr;
        };

        std::string _dev;
        int _fd = -1;
        uint32_t _connector_id = 0;
        uint32_t _crtc_id = 0;
        drmModeCrtc* _saved_crtc = nullptr;
        drmModeModeInfo _mode{};
        DumbBuffer _buffer;

        std::string _backlight_path;
        int _backlight_max = 100;
        bool _backlight_disabled = false;

        void open_device();
        void find_connector();
        void create_framebuffer();
        void set_crtc();
        void destroy_framebuffer();
        void find_backlight_path(const std::string& configured_path);
        void write_pixel(int x, int y, const RGBA& c);
        void mark_dirty();

    public:

        virtual const std::string name() override { return "DRM"; }
        virtual const std::string device() override { return _dev; }
        virtual const int BPP() override;
        virtual ~DRM() override;

        virtual void backlight(int value) override;
        virtual void blit(int x, int y, int width, int height) override;
        virtual void blit_fullscreen() override;
        virtual void clear() override;

        DRM(const std::string& device, int backlight, const std::string& backlight_path, int& width, int& height);
};

}
