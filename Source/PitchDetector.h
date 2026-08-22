#pragma once
#include <JuceHeader.h>
#include <vector>

class PitchDetector
{
public:
    void prepare(double newSampleRate);
    void reset();

    struct Result
    {
        float frequency = 0.0f;
        float confidence = 0.0f;
        float rms = 0.0f;
        bool voiced = false;
    };

    Result process(const float* samples, int numSamples);

private:
    double sampleRate = 44100.0;
    std::vector<float> frame;
    int writePos = 0;
    static constexpr int frameSize = 2048;

    Result analyseFrame() const;
    Result lastResult {};
};
