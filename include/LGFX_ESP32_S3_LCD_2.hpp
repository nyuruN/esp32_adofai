#pragma once

#include <LovyanGFX.hpp>
#include <driver/ledc.h>

// Example configuration for using LovyanGFX with custom settings on ESP32

/// Create a class for custom settings by deriving from LGFX_Device.
class LGFX : public lgfx::LGFX_Device
{
/*
 You can change the class name from "LGFX" to a different name.
 If using with AUTODETECT, "LGFX" is already being used, so please change it to a name other than LGFX.
 Also, if using multiple panels simultaneously, give each a different name.
 ※ If you change the class name, you must also change the constructor name to the same name.

 You are free to name it as you wish, but considering cases where settings increase,
 for example, if you configure ILI9341 with SPI connection on ESP32 DevKit-C,
  LGFX_DevKitC_SPI_ILI9341
 by using such a name and matching the filename and class name, it becomes less confusing when using them.
//*/

// Create an instance of the panel type to be connected.
  lgfx::Panel_ST7789      _panel_instance;

// Create an instance of the bus type to which the panel will be connected.
  lgfx::Bus_SPI        _bus_instance;   // SPI bus instance

// If backlight control is possible, prepare an instance. (Delete if not needed)
  lgfx::Light_PWM     _light_instance;

// Create an instance of the touch screen type. (Delete if not needed)
// none

public:

  // Create a constructor and perform various settings here.
  // If you changed the class name, specify the same name for the constructor as well.
  LGFX(void)
  {
    { // Configure bus control.
      auto cfg = _bus_instance.config();    // Get the structure for bus settings.

// SPI bus settings
      cfg.spi_host = SPI2_HOST;     // Select the SPI to use. ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ With the version upgrade of ESP-IDF, VSPI_HOST and HSPI_HOST are deprecated, so if you get an error, use SPI2_HOST and SPI3_HOST instead.
      cfg.spi_mode = 0;             // Set the SPI communication mode (0 ~ 3)
      cfg.freq_write = 80000000;    // SPI clock during transmission (maximum 80MHz, rounded to a value divided by 80MHz as an integer)
      cfg.freq_read  = 16000000;    // SPI clock during reception
      cfg.spi_3wire  = false;        // Set to true if reception is performed on the MOSI pin
      cfg.use_lock   = true;        // Set to true if using transaction lock
      cfg.dma_channel = SPI_DMA_CH_AUTO; // Set the DMA channel to use (0=No DMA / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=Auto)
      // ※ With the ESP-IDF version upgrade, SPI_DMA_CH_AUTO (Auto) is now recommended for DMA channels. Specifying 1ch, 2ch is deprecated.
      cfg.pin_sclk = 39;            // Set the SPI SCLK pin number
      cfg.pin_mosi = 38;            // Set the SPI MOSI pin number
      cfg.pin_miso = 40;            // Set the SPI MISO pin number (-1 = disable)
      cfg.pin_dc   = 42;            // Set the SPI D/C pin number (-1 = disable)
     // If using a common SPI bus with an SD card, be sure to set MISO without omitting it.
//*/

      _bus_instance.config(cfg);    // Apply settings to the bus.
      _panel_instance.setBus(&_bus_instance);      // Set the bus to the panel.
    }

    { // Configure the display panel control.
      auto cfg = _panel_instance.config();    // Get the structure for display panel settings.

      cfg.pin_cs           =    45;  // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst          =    -1;  // Pin number to which RST is connected (-1 = disable)
      cfg.pin_busy         =    -1;  // Pin number to which BUSY is connected (-1 = disable)

      // ※ The following settings have typical default values set for each panel, so comment out unclear items and try them.

      cfg.panel_width      =   240;  // Actual displayable width
      cfg.panel_height     =   320;  // Actual displayable height
      cfg.offset_x         =     0;  // Panel X-direction offset
      cfg.offset_y         =     0;  // Panel Y-direction offset
      cfg.offset_rotation  =     0;  // Rotation direction value offset 0~7 (4~7 is vertical flip)
      cfg.dummy_read_pixel =     8;  // Number of dummy read bits before pixel read
      cfg.dummy_read_bits  =     1;  // Number of dummy read bits before reading data other than pixels
      cfg.readable         =  true;  // Set to true if data read is possible
      // * Required for IPS panel
      cfg.invert           = true;  // Set to true if the panel's brightness is inverted
      cfg.rgb_order        = false;  // Set to true if the panel's red and blue are swapped
      cfg.dlen_16bit       = false;  // Set to true for panels that transmit data length in 16-bit units with 16-bit parallel or SPI
      cfg.bus_shared       =  true;  // Set to true if sharing a bus with an SD card (bus control is performed in drawJpgFile, etc.)

// Set the following only if the display is misaligned with a driver with variable pixel counts like ST7735 or ILI9163.
//    cfg.memory_width     =   240;  // Maximum width supported by the driver IC
//    cfg.memory_height    =   320;  // Maximum height supported by the driver IC

      _panel_instance.config(cfg);
    }

//*
    { // Set backlight control. (Delete if not needed)
      auto cfg = _light_instance.config();    // Get the structure for backlight settings.

      cfg.pin_bl = 1;              // Pin number to which backlight is connected
      cfg.invert = false;           // Set to true if inverting backlight brightness
      cfg.freq   = 10000;           // PWM frequency of backlight
      cfg.pwm_channel = LEDC_CHANNEL_0;          // PWM channel number to use

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // Set backlight to panel.
    }
//*/

    setPanel(&_panel_instance); // Set the panel to use.
  }
};