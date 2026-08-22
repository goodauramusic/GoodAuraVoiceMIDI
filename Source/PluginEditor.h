#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GoodAuraVoiceMIDIAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GoodAuraVoiceMIDIAudioProcessorEditor(GoodAuraVoiceMIDIAudioProcessor&);
    ~GoodAuraVoiceMIDIAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseExportFile();

    GoodAuraVoiceMIDIAudioProcessor& audioProcessor;

    juce::Label title, status, noteLabel, freqLabel, capturedLabel;
    juce::TextButton recordButton { "Record MIDI" }, clearButton { "Clear" }, exportButton { "Export .mid" };
    juce::Slider sensitivity, confidence, stability, transpose;
    juce::ComboBox root, scale;
    juce::ToggleButton monitor { "Monitor audio" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensitivityAtt, confidenceAtt, stabilityAtt, transposeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootAtt, scaleAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GoodAuraVoiceMIDIAudioProcessorEditor)
};
