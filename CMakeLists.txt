cmake_minimum_required(VERSION 3.22)
project(VeriSpeed VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ──────────────────────────────────────────────────────────────────────
# Expects JUCE to be at ./JUCE or set JUCE_DIR on the command line:
#   cmake -B build -DJUCE_DIR=/path/to/JUCE
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
endif()

if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at ${JUCE_DIR}\n"
        "Clone JUCE 7: git clone https://github.com/juce-framework/JUCE.git --branch 7.0.12\n"
        "or pass -DJUCE_DIR=/path/to/JUCE to cmake")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

# ── Plugin target ──────────────────────────────────────────────────────────────
juce_add_plugin(VeriSpeed
    COMPANY_NAME            "Brian Tushae Thomas"
    PLUGIN_MANUFACTURER_CODE BrTT
    PLUGIN_CODE             VrSp
    FORMATS                 VST3
    PRODUCT_NAME            "VeriSpeed"
    VERSION                 "1.1.0"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         "Fx"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# ── Sources ────────────────────────────────────────────────────────────────────
target_sources(VeriSpeed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/VUMeter.cpp
        Source/PillButton.cpp
)

# ── JUCE module flags ──────────────────────────────────────────────────────────
target_compile_definitions(VeriSpeed
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── Link modules ───────────────────────────────────────────────────────────────
# Generate JuceHeader.h (required by JUCE 8 CMake builds)
juce_generate_juce_header(VeriSpeed)

target_link_libraries(VeriSpeed
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
