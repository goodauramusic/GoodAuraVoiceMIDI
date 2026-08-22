#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

void PitchDetector::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    frame.assign(frameSize, 0.0f);
    writePos = 0;
    lastResult = {};
}

void PitchDetector::reset()
{
    std::fill(frame.begin(), frame.end(), 0.0f);
    writePos = 0;
    lastResult = {};
}

PitchDetector::Result PitchDetector::process(const float* samples, int numSamples)
{
    Result latest = lastResult;

    for (int i = 0; i < numSamples; ++i)
    {
        frame[(size_t) writePos] = samples[i];
        ++writePos;

        if (writePos >= frameSize)
        {
            writePos = 0;
            latest = analyseFrame();
            lastResult = latest;

            // 50% overlap: retain the newest half of the frame
            std::copy(frame.begin() + frameSize / 2, frame.end(), frame.begin());
            std::fill(frame.begin() + frameSize / 2, frame.end(), 0.0f);
            writePos = frameSize / 2;
        }
    }

    return latest;
}

PitchDetector::Result PitchDetector::analyseFrame() const
{
    Result out {};

    double sumSq = 0.0;
    for (auto s : frame)
        sumSq += (double) s * s;

    out.rms = (float) std::sqrt(sumSq / frame.size());

    if (out.rms < 0.008f)
        return out;

    std::vector<float> x(frameSize);
    for (int i = 0; i < frameSize; ++i)
    {
        const float w = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (frameSize - 1));
        x[(size_t)i] = frame[(size_t)i] * w;
    }

    const int minLag = juce::jmax(1, (int) (sampleRate / 1100.0));
    const int maxLag = juce::jmin(frameSize / 2, (int) (sampleRate / 70.0));

    float bestCorr = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double cross = 0.0, e1 = 0.0, e2 = 0.0;
        const int count = frameSize - lag;

        for (int i = 0; i < count; ++i)
        {
            const double a = x[(size_t)i];
            const double b = x[(size_t)(i + lag)];
            cross += a * b;
            e1 += a * a;
            e2 += b * b;
        }

        const double denom = std::sqrt(e1 * e2) + 1.0e-12;
        const float corr = (float)(cross / denom);

        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    if (bestLag == 0 || bestCorr < 0.58f)
        return out;

    auto correlationAt = [&](int lag)
    {
        double cross = 0.0, e1 = 0.0, e2 = 0.0;
        const int count = frameSize - lag;
        for (int i = 0; i < count; ++i)
        {
            const double a = x[(size_t)i];
            const double b = x[(size_t)(i + lag)];
            cross += a * b;
            e1 += a * a;
            e2 += b * b;
        }
        return (float)(cross / (std::sqrt(e1 * e2) + 1.0e-12));
    };

    float lagF = (float) bestLag;
    if (bestLag > minLag && bestLag < maxLag)
    {
        const float ym1 = correlationAt(bestLag - 1);
        const float y0  = bestCorr;
        const float yp1 = correlationAt(bestLag + 1);
        const float d = ym1 - 2.0f * y0 + yp1;
        if (std::abs(d) > 1.0e-6f)
            lagF += 0.5f * (ym1 - yp1) / d;
    }

    out.frequency = (float)(sampleRate / lagF);
    out.confidence = bestCorr;
    out.voiced = out.frequency >= 70.0f && out.frequency <= 1100.0f;
    return out;
}
