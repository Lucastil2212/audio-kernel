cmake_minimum_required(VERSION 3.22.1)
project(manticore_toneflow_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MANTICORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)

set(MANTICORE_SOURCES
    ${MANTICORE_ROOT}/src/dsp/noise.cpp
    ${MANTICORE_ROOT}/src/dsp/filters.cpp
    ${MANTICORE_ROOT}/src/toneflow/tone_oscillator.cpp
    ${MANTICORE_ROOT}/src/toneflow/perception.cpp
    ${MANTICORE_ROOT}/src/toneflow/stereo_reverb.cpp
    ${MANTICORE_ROOT}/src/toneflow/color_filters.cpp
    ${MANTICORE_ROOT}/src/toneflow/presets.cpp
    ${MANTICORE_ROOT}/src/toneflow/tone_engine.cpp

    native_bridge.cpp
    aaudio_player.cpp
)

add_library(manticore_toneflow SHARED ${MANTICORE_SOURCES})

target_include_directories(manticore_toneflow PRIVATE
    ${MANTICORE_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)
find_library(aaudio-lib aaudio)

target_link_libraries(manticore_toneflow
    ${log-lib}
    ${aaudio-lib}
)
