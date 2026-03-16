cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_adofai)

spiffs_create_partition_image(spiffs data FLASH_IN_PROJECT)