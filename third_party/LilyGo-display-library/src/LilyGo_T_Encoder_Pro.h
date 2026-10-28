#pragma once
#include "LilyGo_LovyanGFX_Base.h"

class LilyGo_T_Encoder_Pro : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Panel_SH8601Z _panel_instance;

public:
    LilyGo_T_Encoder_Pro() {
        {
            auto cfg = _bus_instance.config();
            cfg.freq_write  = 75000000;
            cfg.freq_read   = 20000000;
            cfg.pin_sclk    = 12;
            cfg.pin_io0     = 11;
            cfg.pin_io1     = 13;
            cfg.pin_io2     = 7;
            cfg.pin_io3     = 14;
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = SPI_MODE0;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_rst      = 4;
            cfg.pin_cs       = 10;
            cfg.panel_width  = 390;
            cfg.panel_height = 390;
            cfg.readable     = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }

    bool init() {
        lgfx::pinMode(3, lgfx::pin_mode_t::output);
        lgfx::gpio_hi(3);
        return lgfx::LGFX_Device::init();
    }

    bool begin(uint8_t rotation = 0, uint8_t brightness = 255) {
        bool ok = init();
        setRotation(rotation);
        setBrightness(brightness);
        fillScreen(TFT_BLACK);
        return ok;
    }
};
