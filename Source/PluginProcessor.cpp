#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    static int frequencyToMidi(float hz)
    {
        if (hz <= 0.0f) return -1;
        return juce::roundToInt(69.0 + 12.0 * std::log2((double) hz / 440.0));
    }
}

GoodAuraVoiceMIDIAudioProcessor::GoodAuraVoiceMIDIAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::mono(), true)
        .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GoodAuraVoiceMIDIAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sensitivity", 1}, "Sensitivity", juce::NormalisableRange<float>(0.005f, 0.06f, 0.001f), 0.012f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"confidence", 1}, "Pitch Confidence", juce::NormalisableRange<float>(0.45f, 0.9f, 0.01f), 0.62f));
    p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"stability", 1}, "Stability Frames", 1, 6, 2));
    p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"transpose", 1}, "Transpose", -24, 24, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"root", 1}, "Key", juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"scale", 1}, "Scale", juce::StringArray{"Chromatic","Major","Natural Minor","Pentatonic Major","Pentatonic Minor"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"monitor", 1}, "Monitor Audio", true));
    return { p.begin(), p.end() };
}

void GoodAuraVoiceMIDIAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    detector.prepare(sampleRate);
    activeMidiNote = -1;
    pendingMidiNote = -1;
    pendingFrames = silenceFrames = 0;
    sampleCounter = 0;
    captureTimeSeconds = 0.0;
}

bool GoodAuraVoiceMIDIAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && out == in;
}

int GoodAuraVoiceMIDIAudioProcessor::applyScaleLock(int midiNote) const
{
    auto* rootParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("root"));
    auto* scaleParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scale"));
    if (rootParam == nullptr || scaleParam == nullptr) return midiNote;
    const int scale = scaleParam->getIndex();
    if (scale == 0) return midiNote;
    static const std::vector<std::vector<int>> scales = {{},{0,2,4,5,7,9,11},{0,2,3,5,7,8,10},{0,2,4,7,9},{0,3,5,7,10}};
    const int root = rootParam->getIndex();
    int best = midiNote, bestDistance = 128;
    for (int candidate = midiNote - 6; candidate <= midiNote + 6; ++candidate)
    {
        int pc = (candidate - root) % 12;
        if (pc < 0) pc += 12;
        if (std::find(scales[(size_t)scale].begin(), scales[(size_t)scale].end(), pc) != scales[(size_t)scale].end())
        {
            int d = std::abs(candidate - midiNote);
            if (d < bestDistance) { bestDistance = d; best = candidate; }
        }
    }
    return best;
}

void GoodAuraVoiceMIDIAudioProcessor::transitionToNote(int newNote, int velocity, juce::MidiBuffer& midi, int sampleOffset)
{
    if (activeMidiNote == newNote) return;
    if (activeMidiNote >= 0) finishActiveNote(midi, sampleOffset);
    activeMidiNote = newNote;
    midi.addEvent(juce::MidiMessage::noteOn(1, newNote, (juce::uint8)velocity), sampleOffset);
    if (captureEnabled.load())
    {
        activeCaptureNote = newNote;
        activeCaptureStart = captureTimeSeconds + (double)sampleOffset / currentSampleRate;
        activeCaptureVelocity = (juce::uint8)velocity;
    }
}

void GoodAuraVoiceMIDIAudioProcessor::finishActiveNote(juce::MidiBuffer& midi, int sampleOffset)
{
    if (activeMidiNote < 0) return;
    midi.addEvent(juce::MidiMessage::noteOff(1, activeMidiNote), sampleOffset);
    if (captureEnabled.load() && activeCaptureNote >= 0)
    {
        const juce::ScopedLock sl(captureLock);
        captured.push_back({activeCaptureNote, activeCaptureStart, captureTimeSeconds + (double)sampleOffset / currentSampleRate, activeCaptureVelocity});
    }
    activeMidiNote = -1;
    activeCaptureNote = -1;
}

void GoodAuraVoiceMIDIAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() == 0 || numSamples == 0) return;

    juce::AudioBuffer<float> mono(1, numSamples);
    mono.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        mono.addFrom(0, 0, buffer, ch, 0, numSamples, 1.0f / (float)buffer.getNumChannels());

    const auto result = detector.process(mono.getReadPointer(0), numSamples);
    detectedFrequency.store(result.frequency);
    detectedConfidence.store(result.confidence);
    voiced.store(result.voiced);

    const float sensitivity = apvts.getRawParameterValue("sensitivity")->load();
    const float requiredConfidence = apvts.getRawParameterValue("confidence")->load();
    const int stability = (int) apvts.getRawParameterValue("stability")->load();
    const int transpose = (int) apvts.getRawParameterValue("transpose")->load();
    const bool valid = result.voiced && result.rms >= sensitivity && result.confidence >= requiredConfidence;

    if (valid)
    {
        int note = applyScaleLock(frequencyToMidi(result.frequency) + transpose);
        note = juce::jlimit(0, 127, note);
        detectedMidiNote.store(note);
        if (note == pendingMidiNote) ++pendingFrames;
        else { pendingMidiNote = note; pendingFrames = 1; }
        silenceFrames = 0;
        if (pendingFrames >= stability && note != activeMidiNote)
        {
            const int velocity = juce::jlimit(25, 127, (int)juce::jmap(result.rms, sensitivity, 0.25f, 45.0f, 127.0f));
            transitionToNote(note, velocity, midi, 0);
        }
    }
    else
    {
        detectedMidiNote.store(-1);
        voiced.store(false);
        ++silenceFrames;
        pendingFrames = 0;
        pendingMidiNote = -1;
        if (silenceFrames >= 2 && activeMidiNote >= 0) finishActiveNote(midi, 0);
    }

    captureTimeSeconds += (double) numSamples / currentSampleRate;
    sampleCounter += numSamples;
    if (apvts.getRawParameterValue("monitor")->load() <= 0.5f) buffer.clear();
}

void GoodAuraVoiceMIDIAudioProcessor::setCaptureEnabled(bool shouldCapture)
{
    if (captureEnabled.exchange(shouldCapture) == shouldCapture) return;
    if (shouldCapture)
    {
        const juce::ScopedLock sl(captureLock);
        captured.clear();
        captureTimeSeconds = 0.0;
        activeCaptureStart = 0.0;
        activeCaptureNote = -1;
    }
}

void GoodAuraVoiceMIDIAudioProcessor::clearCapture()
{
    const juce::ScopedLock sl(captureLock);
    captured.clear();
}

int GoodAuraVoiceMIDIAudioProcessor::getCapturedNoteCount() const
{
    const juce::ScopedLock sl(captureLock);
    return (int)captured.size();
}

bool GoodAuraVoiceMIDIAudioProcessor::exportCapturedMidi(const juce::File& file)
{
    std::vector<CapturedEvent> copy;
    { const juce::ScopedLock sl(captureLock); copy = captured; }
    if (copy.empty()) return false;
    juce::MidiMessageSequence seq;
    constexpr double bpm = 120.0;
    constexpr int tpq = 960;
    const double beatsPerSecond = bpm / 60.0;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent((int)(60000000.0 / bpm)), 0.0);
    for (const auto& e : copy)
    {
        const double startTicks = e.startSeconds * beatsPerSecond * tpq;
        const double endTicks = juce::jmax(e.startSeconds + 0.03, e.endSeconds) * beatsPerSecond * tpq;
        seq.addEvent(juce::MidiMessage::noteOn(1, e.note, e.velocity), startTicks);
        seq.addEvent(juce::MidiMessage::noteOff(1, e.note), endTicks);
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(tpq);
    mf.addTrack(seq);
    auto stream = file.createOutputStream();
    return stream != nullptr && mf.writeTo(*stream);
}

void GoodAuraVoiceMIDIAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}

void GoodAuraVoiceMIDIAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* GoodAuraVoiceMIDIAudioProcessor::createEditor()
{
    return new GoodAuraVoiceMIDIAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GoodAuraVoiceMIDIAudioProcessor();
}
