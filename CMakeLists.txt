cmake_minimum_required(VERSION 3.22)

project(GoodAuraVoiceMIDI VERSION 0.1.0)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.1
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GoodAuraVoiceMIDI
    COMPANY_NAME "Good Aura Music"
    BUNDLE_ID "com.goodauramusic.voicemidi"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE GaMu
    PLUGIN_CODE GvMd
    FORMATS VST3 Standalone
    PRODUCT_NAME "Good Aura Voice MIDI"
)

juce_generate_juce_header(GoodAuraVoiceMIDI)

target_sources(GoodAuraVoiceMIDI
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PitchDetector.cpp
        Source/PitchDetector.h
)

target_compile_definitions(GoodAuraVoiceMIDI
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GoodAuraVoiceMIDI
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
