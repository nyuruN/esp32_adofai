#pragma once

#if defined(ESP_PLATFORM)
#include "esp_flash.h"

void print_memory_info(void)
{
  // Get total and free sizes for different memory types
  multi_heap_info_t info;

  // 1. Internal SRAM (IRAM/DRAM) - MALLOC_CAP_INTERNAL
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  printf("\n--- Internal SRAM (IRAM/DRAM) ---\n");
  printf("Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
  printf("Free: %d KB\n", info.total_free_bytes / 1024);
  printf("Largest Free Block: %d KB\n", info.largest_free_block / 1024);

  // 2. External PSRAM (SPIRAM) - MALLOC_CAP_SPIRAM
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0)
  {
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    printf("\n--- External PSRAM (SPIRAM) ---\n");
    printf("Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
    printf("Free: %d KB\n", info.total_free_bytes / 1024);
    printf("Largest Free Block: %d KB\n", info.largest_free_block / 1024);
  }
  else
  {
    printf("\n--- External PSRAM (SPIRAM) ---\n");
    printf("PSRAM is not enabled or not detected.\n");
  }

  // 3. Flash Size (Storage, not RAM)
  printf("\n--- Flash (Storage) ---\n");

  uint32_t flash_size;
  auto res = esp_flash_get_size(NULL, &flash_size);

  if (res == ESP_OK)
  {
    printf("Total Flash Size: %d MB\n", (int)flash_size / (1024 * 1024));
  }
  else
  {
    printf("Failed to get flash size.\n");
  }

  // Optional: Print a summary from the main heap
  printf("\n--- Summary ---\n");
  printf("Total Free Heap (all memory): %d KB\n", (int)esp_get_free_heap_size() / 1024);
  printf("Minimum Free Heap Ever: %d KB\n", (int)esp_get_minimum_free_heap_size() / 1024);
}

#endif