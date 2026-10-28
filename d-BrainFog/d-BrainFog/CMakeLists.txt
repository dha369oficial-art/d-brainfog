cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0091 NEW)
# Runtime de C++ enlazado estaticamente: el .vst3 no necesita instalar el VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(dBrainFog VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(dBrainFog
    COMPANY_NAME "DHA369"
    PRODUCT_NAME "d-BrainFog"
    VERSION 1.0.0
    PLUGIN_MANUFACTURER_CODE Dha3
    PLUGIN_CODE Dbf1
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    BUNDLE_ID com.dha369.dbrainfog
    DESCRIPTION "d-BrainFog - post-punk / darkwave synth by DHA369")

target_sources(dBrainFog PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/BrainFogEngine.h
    Source/DSP/ParamDefs.h
    Source/DSP/Presets.h)

target_compile_definitions(dBrainFog PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(dBrainFog
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Pruebas del motor DSP (sin JUCE):  cmake -DBUILD_DSP_TESTS=ON
option(BUILD_DSP_TESTS "Compilar pruebas del motor DSP" OFF)
if(BUILD_DSP_TESTS)
    add_executable(dbf_dsp_test tests/dsp_test.cpp)
    add_executable(dbf_export_presets tests/export_presets.cpp)
endif()
