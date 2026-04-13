cmake_minimum_required(VERSION 3.12)
project (lv_wasm)

add_definitions(-DLGFX_SDL)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -g -s USE_SDL=2")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(../../../LovyanGFX/src/)
include_directories(include/)

file(GLOB MY_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS 
    LGFX_WASM.cpp
    src/*.cpp
    ../../../LovyanGFX/src/lgfx/Fonts/efont/*.c
    ../../../LovyanGFX/src/lgfx/Fonts/IPA/*.c
    ../../../LovyanGFX/src/lgfx/utility/*.c
    ../../../LovyanGFX/src/lgfx/v1/*.cpp
    ../../../LovyanGFX/src/lgfx/v1/misc/*.cpp
    ../../../LovyanGFX/src/lgfx/v1/panel/Panel_Device.cpp
    ../../../LovyanGFX/src/lgfx/v1/panel/Panel_FrameBufferBase.cpp
    ../../../LovyanGFX/src/lgfx/v1/platforms/sdl/*.cpp
    )
set(SOURCES ${MY_SOURCES})

add_executable(index ${SOURCES} ${INCLUDES})

#target_link_libraries(index -lembind)
set(CMAKE_EXECUTABLE_SUFFIX ".js")
set_target_properties(index PROPERTIES LINK_FLAGS
    "-s SINGLE_FILE=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1 -s EXPORT_NAME=\"createModule\" -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"HEAPU8\"]' -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='[\"_malloc\", \"_free\", \"_main\"]'"
)