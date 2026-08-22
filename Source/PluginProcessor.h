#pragma once

#include <JuceHeader.h>
#include "PitchDetector.h"
#include <deque>

class GoodAuraVoiceMIDIAudioProcessor : public juce::AudioProcessor
{
public:
    GoodAuraVoiceMIDIAudioProcessor();
    ~GoodAuraVoiceMIDIAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    float getDetectedFrequency() const { return detectedFrequency.load(); }
    float getDetectedConfidence() const { return detectedConfidence.load(); }
    int getDetectedMidiNote() const { return detectedMidiNote.load(); }
    bool getVoiced() const { return voiced.load(); }

    void setCaptureEnabled(bool shouldCapture);
    bool isCaptureEnabled() const { return captureEnabled.load(); }
    void clearCapture();
    int getCapturedNoteCount() const;
    bool exportCapturedMidi(const juce::File& file);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    PitchDetector detector;
    double currentSampleRate = 44100.0;

    std::atomic<float> detectedFrequency { 0.0f };
    std::atomic<float> detectedConfidence { 0.0f };
    std::atomic<int> detectedMidiNote { -1 };
    std::atomic<bool> voiced { false };
    std::atomic<bool> captureEnabled { false };

    int activeMidiNote = -1;
    int pendingMidiNote = -1;
    int pendingFrames = 0;
    int silenceFrames = 0;
    int sampleCounter = 0;

    struct CapturedEvent
    {
        int note = 60;
        double startSeconds = 0.0;
        double endSeconds = 0.0;
        uint8 velocity = 100;
    };

    mutable juce::CriticalSection captureLock;
    std::vector<CapturedEvent> captured;
    double captureTimeSeconds = 0.0;
    double activeCaptureStart = 0.0;
    int activeCaptureNote = -1;
    uint8 activeCaptureVelocity = 100;

    int applyScaleLock(int midiNote) const;
    void transitionToNote(int newNote, int velocity, juce::MidiBuffer& midi, int sampleOffset);
    void finishActiveNote(juce::MidiBuffer& midi, int sampleOffset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GoodAuraVoiceMIDIAudioProcessor)
};
