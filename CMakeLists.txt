cmake_minimum_required(VERSION 3.16)
project(manticore-audio-kernel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MANTICORE_BUILD_TESTS "Build unit tests" ON)
option(MANTICORE_ENABLE_PORTAUDIO "Enable PortAudio live backend" ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(MANTICORE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(MANTICORE_SOURCES
    src/core/time.cpp
    src/dsp/gain.cpp
    src/dsp/limiter.cpp
    src/dsp/analyzer.cpp
    src/dsp/oscillator.cpp
    src/dsp/noise.cpp
    src/dsp/filters.cpp
    src/dsp/delay_line.cpp
    src/dsp/mixer.cpp
    src/dsp/envelope.cpp
    src/dsp/wav_writer.cpp
    src/telemetry/system_health.cpp
    src/therapy/therapy_param_set.cpp
    src/therapy/stimulus_engine.cpp
    src/toneflow/tone_oscillator.cpp
    src/toneflow/perception.cpp
    src/toneflow/stereo_reverb.cpp
    src/toneflow/color_filters.cpp
    src/toneflow/presets.cpp
    src/toneflow/tone_engine.cpp

    src/safety/safety_governor.cpp
    src/engine/dsp_graph.cpp
    src/engine/audio_engine.cpp
    src/io/portaudio_backend.cpp
)

add_library(manticore_audio STATIC ${MANTICORE_SOURCES})
target_include_directories(manticore_audio PUBLIC ${MANTICORE_INCLUDE_DIR})

if(MANTICORE_ENABLE_PORTAUDIO)
    find_package(PortAudio QUIET)
    if(PortAudio_FOUND OR TARGET PortAudio::PortAudio)
        target_compile_definitions(manticore_audio PUBLIC MANTICORE_ENABLE_PORTAUDIO)
        if(TARGET PortAudio::PortAudio)
            target_link_libraries(manticore_audio PUBLIC PortAudio::PortAudio)
        else()
            target_include_directories(manticore_audio PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
            target_link_libraries(manticore_audio PUBLIC ${PORTAUDIO_LIBRARIES})
        endif()
    else()
        find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
        find_library(PORTAUDIO_LIBRARY NAMES portaudio)
        if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
            target_compile_definitions(manticore_audio PUBLIC MANTICORE_ENABLE_PORTAUDIO)
            target_include_directories(manticore_audio PRIVATE ${PORTAUDIO_INCLUDE_DIR})
            target_link_libraries(manticore_audio PUBLIC ${PORTAUDIO_LIBRARY})
        else()
            message(STATUS "PortAudio not found; live tools will be stubbed")
        endif()
    endif()
endif()

add_executable(manticore_main src/main.cpp)
target_link_libraries(manticore_main PRIVATE manticore_audio)

add_executable(generate_sine tools/generate_sine.cpp)
target_link_libraries(generate_sine PRIVATE manticore_audio)

add_executable(generate_noise tools/generate_noise.cpp)
target_link_libraries(generate_noise PRIVATE manticore_audio)

add_executable(inspect_wav tools/inspect_wav.cpp)
target_link_libraries(inspect_wav PRIVATE manticore_audio)

add_executable(live_passthrough tools/live_passthrough.cpp)
target_link_libraries(live_passthrough PRIVATE manticore_audio)

add_executable(latency_click_test tools/latency_click_test.cpp)
target_link_libraries(latency_click_test PRIVATE manticore_audio)

add_executable(generate_toneflow tools/generate_toneflow.cpp)
target_link_libraries(generate_toneflow PRIVATE manticore_audio)

if(MANTICORE_BUILD_TESTS)
    set(MANTICORE_TEST_SOURCES
        tests/test_main.cpp
        tests/test_audio_frame.cpp
        tests/test_gain.cpp
        tests/test_limiter.cpp
        tests/test_analyzer.cpp
        tests/test_oscillator.cpp
        tests/test_wav_writer.cpp
        tests/test_filters.cpp
        tests/test_delay_line.cpp
        tests/test_safety_governor.cpp
        tests/test_dsp_graph.cpp
    )

    add_executable(manticore_tests ${MANTICORE_TEST_SOURCES})
    target_link_libraries(manticore_tests PRIVATE manticore_audio)
    add_test(NAME manticore_tests COMMAND manticore_tests)
endif()

enable_testing()
