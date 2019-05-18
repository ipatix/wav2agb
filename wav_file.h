#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <limits>

#define WAV_INVALID_VAL 0xFFFFFFFFu

class wav_file {
public:
    wav_file(const std::string& path);
    ~wav_file();

    uint32_t getLoopStart() const { return loopStart; }
    uint32_t getLoopEnd() const { return loopEnd; }
    bool getLoopEnabled() const { return loopEnabled; }
    double getTuning() const { return tuning; }
    uint8_t getMidiKey() const { return midiKey; }
    uint32_t getSampleRate() const { return sampleRate; }
    void readData(size_t location, double *data, size_t len);
private:

    std::ifstream ifs;
    std::streampos dataChunkPos;
    std::streampos dataChunkEndPos;
    size_t loadedChunk = WAV_INVALID_VAL;

    std::vector<double> loadBuffer;
    size_t loadBufferBlock = std::numeric_limits<size_t>::max();
    enum class format_type {
        u8, s16, s24, s32,
        f32, f64,
    } fmt;
    uint32_t fmt_size() const;
    uint32_t loopStart = 0; // samples
    uint32_t loopEnd = std::numeric_limits<uint32_t>::max();   // samples
    bool loopEnabled = false;
    double tuning = 0.0;    // cents
    uint8_t midiKey = 60;
    uint32_t sampleRate;
};
