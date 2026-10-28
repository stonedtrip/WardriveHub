#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T_Display_Bar : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LilyGo_T_Display_Bar() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host   = SPI2_HOST;
            cfg.spi_mode   = SPI_MODE0;
            cfg.freq_write = 40000000;
            cfg.freq_read  = 20000000;
            cfg.pin_sclk   = 7;
            cfg.pin_mosi   = 6;
            cfg.pin_miso   = -1;
            cfg.pin_dc     = 5;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs        = 8;
            cfg.pin_rst       = 40;
            cfg.pin_busy      = -1;
            cfg.memory_width  = 76;
            cfg.memory_height = 284;
            cfg.panel_width   = 76;
            cfg.panel_height  = 284;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            cfg.invert        = false;
            cfg.rgb_order     = false;
            cfg.bus_shared    = true;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 15;
            cfg.invert      = true;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }

    bool begin(uint8_t rotation = 0, uint8_t brightness = 255) {
        bool ok = init();
        setRotation(rotation);
        setBrightness(brightness);
        fillScreen(TFT_BLACK);
        return ok;
    }
};
