#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T_Camera_Plus_S3 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_CST816S _touch_instance;

public:
    explicit LilyGo_T_Camera_Plus_S3(bool use_v12_pins = true) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = SPI_MODE0;
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 16000000;
            cfg.pin_sclk    = use_v12_pins ? 35 : 36;
            cfg.pin_mosi    = use_v12_pins ? 34 : 35;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = 45;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = use_v12_pins ? 36 : 34;
            cfg.pin_rst          = -1;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 240;
            cfg.memory_height    = 320;
            cfg.panel_width      = 240;
            cfg.panel_height     = 240;
            cfg.offset_x         = 0;
            cfg.offset_y         = 80;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 16;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = true;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 46;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        {
            auto cfg = _touch_instance.config();
            cfg.i2c_port = 0;
            cfg.i2c_addr = 0x15;
            cfg.pin_sda  = use_v12_pins ? 33 : 1;
            cfg.pin_scl  = use_v12_pins ? 37 : 2;
            cfg.pin_int  = 47;
            cfg.pin_rst  = use_v12_pins ? -1 : 48;
            cfg.freq     = 400000;
            cfg.x_min    = 0;
            cfg.x_max    = 239;
            cfg.y_min    = 0;
            cfg.y_max    = 239;
            cfg.offset_rotation = 0;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
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
