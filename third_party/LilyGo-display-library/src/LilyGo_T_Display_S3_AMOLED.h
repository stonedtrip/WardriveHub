#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T_Display_S3_AMOLED : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Panel_RM67162 _panel_instance;

public:
    LilyGo_T_Display_S3_AMOLED() {
        {
            auto cfg = _bus_instance.config();
            cfg.freq_write  = 75000000;
            cfg.freq_read   = 20000000;
            cfg.pin_sclk    = 5;
            cfg.pin_io0     = 47;
            cfg.pin_io1     = 41;
            cfg.pin_io2     = 0;
            cfg.pin_io3     = 42;
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = SPI_MODE0;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_rst      = 17;
            cfg.pin_cs       = 6;
            cfg.panel_width  = 240;
            cfg.panel_height = 536;
            cfg.readable     = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }

    bool init() {
        lgfx::pinMode(38, lgfx::pin_mode_t::output);
        lgfx::gpio_hi(38);
        lgfx::pinMode(9, lgfx::pin_mode_t::output);
        lgfx::gpio_hi(9);
        return lgfx::LGFX_Device::init();
    }

    bool begin(uint8_t rotation = 1, uint8_t brightness = 255) {
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
            setPanel(&_panel_instance);
        }
    }
};
