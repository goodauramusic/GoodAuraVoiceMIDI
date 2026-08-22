#include "PluginEditor.h"

GoodAuraVoiceMIDIAudioProcessorEditor::GoodAuraVoiceMIDIAudioProcessorEditor(GoodAuraVoiceMIDIAudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setSize(720, 500);
    title.setText("GOOD AURA VOICE MIDI", juce::dontSendNotification);
    title.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    status.setJustificationType(juce::Justification::centred);
    noteLabel.setJustificationType(juce::Justification::centred);
    noteLabel.setFont(juce::FontOptions(48.0f, juce::Font::bold));
    freqLabel.setJustificationType(juce::Justification::centred);
    capturedLabel.setJustificationType(juce::Justification::centred);
    for (auto* c : { &title, &status, &noteLabel, &freqLabel, &capturedLabel, &recordButton, &clearButton, &exportButton, &sensitivity, &confidence, &stability, &transpose, &root, &scale, &monitor }) addAndMakeVisible(c);
    auto configureSlider = [](juce::Slider& s, const juce::String& suffix) { s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 20); s.setTextValueSuffix(suffix); };
    configureSlider(sensitivity, ""); configureSlider(confidence, ""); configureSlider(stability, ""); configureSlider(transpose, " st");
    root.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    scale.addItemList({"Chromatic","Major","Natural Minor","Pentatonic Major","Pentatonic Minor"}, 1);
    sensitivityAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "sensitivity", sensitivity);
    confidenceAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "confidence", confidence);
    stabilityAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "stability", stability);
    transposeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "transpose", transpose);
    rootAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "root", root);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "scale", scale);
    monitorAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "monitor", monitor);
    recordButton.onClick = [this] { const bool next = !processor.isCaptureEnabled(); processor.setCaptureEnabled(next); recordButton.setButtonText(next ? "Stop Recording" : "Record MIDI"); };
    clearButton.onClick = [this] { processor.clearCapture(); };
    exportButton.onClick = [this] { chooseExportFile(); };
    startTimerHz(20);
}

void GoodAuraVoiceMIDIAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(18,18,22));
    auto bounds = getLocalBounds().reduced(18);
    g.setColour(juce::Colour::fromRGB(55,55,65));
    g.drawRoundedRectangle(bounds.toFloat(), 16.0f, 1.5f);
    g.setColour(juce::Colour::fromRGB(190,190,200));
    g.setFont(13.0f);
    g.drawText("Sing or hum a single-note melody into the audio input. The VST3 emits MIDI in real time.", 35,55,getWidth()-70,24,juce::Justification::centred);
    g.drawText("Sensitivity",40,305,120,20,juce::Justification::centred); g.drawText("Confidence",185,305,120,20,juce::Justification::centred); g.drawText("Stability",330,305,120,20,juce::Justification::centred); g.drawText("Transpose",475,305,120,20,juce::Justification::centred); g.drawText("Key",70,405,80,20,juce::Justification::centred); g.drawText("Scale",255,405,140,20,juce::Justification::centred);
}

void GoodAuraVoiceMIDIAudioProcessorEditor::resized()
{
    title.setBounds(30,18,getWidth()-60,40); status.setBounds(60,90,getWidth()-120,28); noteLabel.setBounds(80,120,getWidth()-160,70); freqLabel.setBounds(80,185,getWidth()-160,26); capturedLabel.setBounds(80,215,getWidth()-160,24);
    recordButton.setBounds(110,255,150,34); clearButton.setBounds(285,255,120,34); exportButton.setBounds(430,255,150,34);
    sensitivity.setBounds(45,325,110,75); confidence.setBounds(190,325,110,75); stability.setBounds(335,325,110,75); transpose.setBounds(480,325,110,75);
    root.setBounds(60,430,120,30); scale.setBounds(220,430,210,30); monitor.setBounds(480,430,150,30);
}

void GoodAuraVoiceMIDIAudioProcessorEditor::timerCallback()
{
    const bool isVoiced = processor.getVoiced(); const float hz = processor.getDetectedFrequency(); const int midi = processor.getDetectedMidiNote();
    status.setText(isVoiced ? "VOICE DETECTED" : "Listening...", juce::dontSendNotification);
    if (midi >= 0) { noteLabel.setText(juce::MidiMessage::getMidiNoteName(midi,true,true,3), juce::dontSendNotification); freqLabel.setText(juce::String(hz,1)+" Hz", juce::dontSendNotification); }
    else { noteLabel.setText("--", juce::dontSendNotification); freqLabel.setText("", juce::dontSendNotification); }
    capturedLabel.setText("Captured notes: "+juce::String(processor.getCapturedNoteCount()), juce::dontSendNotification);
}

void GoodAuraVoiceMIDIAudioProcessorEditor::chooseExportFile()
{
    auto chooser = std::make_shared<juce::FileChooser>("Export captured melody", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("GoodAuraVoice.mid"), "*.mid");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this, chooser](const juce::FileChooser& fc) { auto f = fc.getResult(); if (f != juce::File()) { if (f.getFileExtension().isEmpty()) f = f.withFileExtension(".mid"); processor.exportCapturedMidi(f); } });
}
