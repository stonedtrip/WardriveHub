#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T4_S3 : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Panel_RM690B0 _panel_instance;
    lgfx::Touch_CST226  _touch_instance;
    uint8_t             _offset_rotation = 0;

public:
    LilyGo_T4_S3() {
        {
            auto cfg = _bus_instance.config();
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 20000000;
            cfg.pin_sclk    = 15;
            cfg.pin_io0     = 14;
            cfg.pin_io1     = 10;
            cfg.pin_io2     = 16;
            cfg.pin_io3     = 12;
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = SPI_MODE0;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_rst      = 13;
            cfg.pin_cs       = 11;
            cfg.panel_width  = 452;
            cfg.panel_height = 600;
            cfg.readable     = true;
            cfg.offset_x     = 16;
            cfg.offset_y     = 0;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _touch_instance.config();
            cfg.i2c_addr        = 0x5A;
            cfg.pin_sda         = 6;
            cfg.pin_scl         = 7;
            cfg.pin_int         = 8;
            cfg.pin_rst         = 17;
            cfg.x_min           = 0;
            cfg.y_min           = 0;
            cfg.x_max           = 450;
            cfg.y_max           = 600;
            cfg.freq            = 400000;
            cfg.bus_shared      = false;
            cfg.offset_rotation = _offset_rotation = 0;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }

    bool init() {
        lgfx::pinMode(9, lgfx::pin_mode_t::output);
        lgfx::gpio_hi(9);
        return lgfx::LGFX_Device::init();
    }

    bool begin(uint8_t rotation = 0, uint8_t brightness = 255) {
        bool ok = init();
        setRotation(rotation);
        setBrightness(brightness);
        fillScreen(TFT_BLACK);
        return ok;
    }

    bool enableFrameBuffer(bool auto_display = false) {
        if (_panel_instance.initPanelFb()) {
            auto fbPanel = _panel_instance.getPanelFb();
            if (fbPanel) {
                fbPanel->setBus(&_bus_instance);
                auto touch_cfg = _touch_instance.config();
                _offset_rotation = touch_cfg.offset_rotation;
                touch_cfg.offset_rotation = _panel_instance.getRotation();
                if (touch_cfg.offset_rotation & 1) {
                    uint16_t max = touch_cfg.x_max;
                    touch_cfg.x_max = touch_cfg.y_max;
                    touch_cfg.y_max = max;
                }
                _touch_instance.config(touch_cfg);
                fbPanel->setTouch(&_touch_instance);
                fbPanel->setAutoDisplay(auto_display);
                setPanel(fbPanel);
                return true;
            }
        }
        return false;
    }

    void disableFrameBuffer() {
        auto fbPanel = _panel_instance.getPanelFb();
        if (fbPanel) {
            _panel_instance.deinitPanelFb();
            auto touch_cfg = _touch_instance.config();
            if (touch_cfg.offset_rotation & 1) {
                uint16_t max = touch_cfg.x_max;
                touch_cfg.x_max = touch_cfg.y_max;
                touch_cfg.y_max = max;
            }
            touch_cfg.offset_rotation = _offset_rotation;
            _touch_instance.config(touch_cfg);
            _panel_instance.setTouch(&_touch_instance);
            setPanel(&_panel_instance);
        }
    }
};
