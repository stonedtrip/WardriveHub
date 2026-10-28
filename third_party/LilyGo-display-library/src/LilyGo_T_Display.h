#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T_Display : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LilyGo_T_Display() {
        {
            auto cfg = _bus_instance.config();
#if defined(VSPI_HOST)
            cfg.spi_host   = VSPI_HOST;
#elif defined(SPI3_HOST)
            cfg.spi_host   = SPI3_HOST;
#else
            cfg.spi_host   = SPI2_HOST;
#endif
            cfg.spi_mode   = SPI_MODE3;
            cfg.freq_write = 40000000;
            cfg.freq_read  = 6000000;
            cfg.pin_sclk   = 18;
            cfg.pin_mosi   = 19;
            cfg.pin_miso   = -1;
            cfg.pin_dc     = 16;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 5;
            cfg.pin_rst          = 23;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 240;
            cfg.memory_height    = 320;
            cfg.panel_width      = 135;
            cfg.panel_height     = 240;
            cfg.offset_x         = 52;
            cfg.offset_y         = 40;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 16;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = true;
            cfg.invert           = true;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = true;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 4;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }

    bool begin(uint8_t rotation = 1, uint8_t brightness = 255) {
        bool ok = init();
        setRotation(rotation);
        setBrightness(brightness);
        fillScreen(TFT_BLACK);
        return ok;
    }
};
