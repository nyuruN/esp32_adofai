#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "LGFX_ESP32_S3_LCD_2.hpp"

// Covenient Rust-like type definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Global objects
static LGFX lcd;
static LGFX_Sprite _sprites[2];
static LGFX_Sprite _background;

// Auxiliary variables
static std::uint32_t _fps = 0;
static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
bool _is_running;
std::uint32_t _draw_count;
std::uint32_t _loop_count;

// Fixed point math variables
static constexpr std::uint32_t SHIFTSIZE = 8;
static std::uint32_t _width;
static std::uint32_t _height;

// Perform partial refresh
static void diffDraw(LGFX_Sprite* sp0, LGFX_Sprite* sp1)
{
  union
  {
    std::uint32_t* s32;
    std::uint8_t* s;
  };
  union
  {
    std::uint32_t* p32;
    std::uint8_t* p;
  };
  s32 = (std::uint32_t*)sp0->getBuffer();
  p32 = (std::uint32_t*)sp1->getBuffer();

  auto width  = sp0->width();
  auto height = sp0->height();

  auto w32 = (width+3) >> 2;
  std::int32_t y = 0;
  do
  {
    std::int32_t x32 = 0;
    do
    {
      while (s32[x32] == p32[x32] && ++x32 < w32);
      if (x32 == w32) break;

      std::int32_t xs = x32 << 2;
      while (s[xs] == p[xs]) ++xs;

      while (++x32 < w32 && s32[x32] != p32[x32]);

      std::int32_t xe = (x32 << 2) - 1;
      if (xe >= width) xe = width - 1;
      while (s[xe] == p[xe]) --xe;

      lcd.pushImage(xs, y, xe - xs + 1, 1, &s[xs]);
    } while (x32 < w32);
    s32 += w32;
    p32 += w32;
  } while (++y < height);
  lcd.display();
}

static void drawfunc(void)
{
  LGFX_Sprite *sprite;

  auto width  = _sprites[0].width();
  auto height = _sprites[0].height();

  std::size_t flip = _draw_count & 1;

  sprite = &(_sprites[flip]);
  sprite->clear();

  
  _background.pushSprite(sprite, 0, 0);

  sprite->fillCircle(width / 2, height / 2, (height / 2 - 20) * (_loop_count % 256) / 256, sprite->color332(0, 0, 0));
  sprite->fillCircle(width / 2, height / 2, ((height - 20) / 2 - 20) * (_loop_count % 256) / 256, sprite->color332(255, 255, 255));

  sprite->setCursor(1,1);
  sprite->setTextColor(TFT_BLACK);
  sprite->printf("fps:%d", (int)_fps);
  sprite->setCursor(0,0);
  sprite->setTextColor(TFT_WHITE);
  sprite->printf("fps:%d", (int)_fps);

  diffDraw(&_sprites[flip], &_sprites[!flip]);
  ++_draw_count;
}

static void mainfunc(void)
{
  sec = lgfx::millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
    vTaskDelay(1);
  }

  frame_count++;
  _loop_count++;
  _fps = fps;
}

void setup_display(void)
{
  lcd.init();

  lcd.startWrite();
  lcd.setColorDepth(8);
  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

  auto lcd_width = lcd.width();
  auto lcd_height = lcd.height();

  for (std::uint32_t i = 0; i < 2; ++i)
  {
    _sprites[i].setTextSize(2);
    _sprites[i].setColorDepth(8);
  }

  bool fail = false;
  for (std::uint32_t i = 0; !fail && i < 2; ++i)
  {
    fail = !_sprites[i].createSprite(lcd_width, lcd_height);
  }

  if (fail)
  {
    fail = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i)
    {
      _sprites[i].setPsram(true);
      fail = !_sprites[i].createSprite(lcd_width, lcd_height);
    }

    if (fail)
    {
      fail = false;
      if (lcd_width > 320) lcd_width = 320;
      if (lcd_height > 240) lcd_height = 240;

      for (std::uint32_t i = 0; !fail && i < 2; ++i)
      {
        _sprites[i].setPsram(true);
        fail = !_sprites[i].createSprite(lcd_width, lcd_height);
      }
      if (fail)
      {
        lcd.print("createSprite fail...");
        lgfx::delay(3000);
      }
    }
  }

  _width = lcd_width << SHIFTSIZE;
  _height = lcd_height << SHIFTSIZE;

  _is_running = true;
  _draw_count = 0;
  _loop_count = 0;
}

#include "esp_flash.h"
#include "esp_log.h"

void print_memory_info(void) {
    // Get total and free sizes for different memory types
    multi_heap_info_t info;

    // 1. Internal SRAM (IRAM/DRAM) - MALLOC_CAP_INTERNAL
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    ESP_LOGI("Main", "\n--- Internal SRAM (IRAM/DRAM) ---\n");
    ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
    ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
    ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);

//size_t psram_size = esp_psram_get_size();
//ESP_LOGI("Main", "PSRAM size: %d bytes\n", (int)psram_size);

    // 2. External PSRAM (SPIRAM) - MALLOC_CAP_SPIRAM
    if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0) {
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
        ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
        ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
        ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);
    } else {
        ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
        ESP_LOGI("Main", "PSRAM is not enabled or not detected.\n");
    }

    // 3. Flash Size (Storage, not RAM)
    ESP_LOGI("Main", "\n--- Flash (Storage) ---\n");

    uint32_t flash_size;
    auto res = esp_flash_get_size(NULL, &flash_size);

    if (res == ESP_OK) {
      ESP_LOGI("Main", "Total Flash Size: %d MB\n", (int)flash_size / (1024 * 1024));
    } else {
      ESP_LOGI("Main", "Failed to get flash size.\n");
    }

    // Optional: Print a summary from the main heap
    ESP_LOGI("Main", "\n--- Summary ---\n");
    ESP_LOGI("Main", "Total Free Heap (all memory): %d KB\n", (int)esp_get_free_heap_size() / 1024);
    ESP_LOGI("Main", "Minimum Free Heap Ever: %d KB\n", (int)esp_get_minimum_free_heap_size() / 1024);
}

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

void setup_sdcard(void) {
  esp_log_level_set("sdmmc", ESP_LOG_VERBOSE);
  esp_log_level_set("sdspi", ESP_LOG_VERBOSE);

  esp_err_t ret;

  // 2. Configure the SD slot (SPI mode)
  sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_cfg.gpio_cs = (gpio_num_t)41;
  slot_cfg.host_id = SPI2_HOST;
  
  // 3. Configure the SDMMC host structure for SPI
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST; // Use the same SPI host
  //host.max_freq_khz = SDMMC_FREQ_DEFAULT; // 20MHz, which is the max for SPI mode
  host.max_freq_khz = 400;

  /*
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = 38,
      .miso_io_num = 40,
      .sclk_io_num = 39,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000
  };

  ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK) {
      ESP_LOGE("sd", "Failed to initialize bus.");
      return;
  }
  */

  delay(1000); // Short delay to ensure bus is ready
  
  // 4. Mount the filesystem
  esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = false, // Set to true to format the card if mounting fails
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };
  
  sdmmc_card_t *card;
  ret = esp_vfs_fat_sdspi_mount("/sd", &host, &slot_cfg, &mount_config, &card);
  
  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE("sd", "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
      } else {
          ESP_LOGE("sd", "Failed to initialize the SD card (%s). Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      }
      esp_vfs_fat_sdcard_unmount("/sd", card);
      return;
  }
  
  // 5. Card initialization successful, print card info
  sdmmc_card_print_info(stdout, card);

  uint8_t buffer[512];
  esp_err_t err = sdmmc_read_sectors(card, buffer, 0, 1);
  if (err != ESP_OK) {
    ESP_LOGE("sd", "Raw read failed: 0x%x", err);
  } else {
    ESP_LOGI("sd", "Raw read successful. First 16 bytes:");
    for (int i = 0; i < 16; i++) {
        ESP_LOGI("sd", "%02x ", buffer[i]);
    }
    ESP_LOGI("sd", "\n");
  }

  esp_vfs_fat_sdcard_unmount("/sd", card);

  // Create a temporary "dummy" device on the same SPI bus.
  // Using spics_io_num = -1 means no CS pin is driven.
  spi_device_handle_t dummy_dev;
  spi_device_interface_config_t dummy_cfg = {
      .mode = 0,                     // SPI mode 0 (CPOL=0, CPHA=0)
      .clock_speed_hz = 1000000,     // 1 MHz – safe and fast enough
      .spics_io_num = -1,            // No CS pin
      .queue_size = 1,
  };
  spi_bus_add_device(SPI2_HOST, &dummy_cfg, &dummy_dev);

  // Send 10 dummy bytes (0xFF) – usually 8–16 bytes are enough.
  uint8_t dummy_data[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  spi_transaction_t trans = {
      .flags = SPI_TRANS_USE_TXDATA, // optional
      .length = 8 * 10,   // 10 bytes
      .tx_buffer = dummy_data,
  };
  spi_device_transmit(dummy_dev, &trans);

  // Remove the dummy device to free resources.
  spi_bus_remove_device(dummy_dev);
}

#include <LittleFS.h>
#include <Arduino.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>

AudioGeneratorMP3 *mp3;
AudioFileSourceLittleFS *file;
AudioOutputI2S *out;

void setup(void) {
  setup_display();
  setup_sdcard();
  LittleFS.begin(true, "/littlefs", 10, "littlefs");

  _background.setTextSize(2);
  _background.setColorDepth(8);
  _background.setPsram(true);
  _background.createSprite(lcd.width(), lcd.height());
  _background.drawPngFile("/littlefs/bg.png", 0, 0, lcd.width(), lcd.height());

  file = new AudioFileSourceLittleFS("/audio.mp3");
  out = new AudioOutputI2S();
  mp3 = new AudioGeneratorMP3();

  out->SetPinout(12, 11, 14);
  out->SetGain(0.05);

  mp3->begin(file, out);

  print_memory_info();
}

void loop(void) {
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
    }
  }
  mainfunc();
  drawfunc();
}