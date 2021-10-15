#include "converter.h"

#include <stdexcept>
#include <fstream>
#include <algorithm>

#include <cmath>
#include <cstdarg>
#include <cassert>
#include <cstring>
#include <ctime>

#include "wav_file.h"

static void agb_out(std::ofstream& ofs, const char *msg, ...) {
    char buf[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);
    ofs << buf;
}

static void data_write(std::ofstream& ofs, uint32_t& block_pos, int data, bool hex) {
    if (block_pos++ == 0) {
        if (hex)
            agb_out(ofs, "\n    .byte   0x%02X", data);
        else
            agb_out(ofs, "\n    .byte   %4d", data);
    } else {
        if (hex)
            agb_out(ofs, ", 0x%02X", data);
        else
            agb_out(ofs, ", %4d", data);
    }
    block_pos %= 16;
}

template<typename T>
const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

static void convert_uncompressed(wav_file& wf, std::ofstream& ofs)
{
    int loop_sample = 0;

    uint32_t block_pos = 0;

    for (size_t i = 0; i < wf.loopEnd; i++) {
        double ds;
        wf.readData(i, &ds, 1);
        // TODO apply dither noise
        int s = clamp(static_cast<int>(floor(ds * 128.0)), -128, 127);

        if (wf.loopEnabled && i == wf.loopStart)
            loop_sample = s;

        data_write(ofs, block_pos, s, false);
    }

    data_write(ofs, block_pos, loop_sample, false);
}

static uint32_t wav_loop_start;
static bool wav_loop_start_override = false;
static uint32_t wav_loop_end;
static bool wav_loop_end_override = false;
static double wav_tune;
static bool wav_tune_override = false;
static uint8_t wav_key;
static bool wav_key_override = false;
static uint32_t wav_rate;
static bool wav_rate_override = false;

static bool dpcm_verbose = false;
static bool dpcm_lookahead_fast = false;
static size_t dpcm_enc_lookahead = 3;
static const size_t DPCM_BLK_SIZE = 0x40;
static const std::vector<int8_t> dpcmLookupTable = { 
    0, 1, 4, 9, 16, 25, 36, 49, -64, -49, -36, -25, -16, -9, -4, -1 
};
static const std::vector<size_t> dpcmIndexTable = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static const std::vector<const std::vector<size_t>> dpcmFastLookupTable = { 
    {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {8, 9}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {9, 10}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {10, 11}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {11, 12}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {13, 14}, {13, 14}, {13, 14}, {13, 14}, {13, 14}, {14, 15}, {14, 15}, {14, 15}, {0, 15}, {0, 1, 15}, {1, 0}, {1, 2}, {1, 2}, {2, 1}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {3, 2}, {3, 4}, {3, 4}, {3, 4}, {3, 4}, {3, 4}, {3, 4}, {4, 3}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {5, 4}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {5, 6}, {6, 5}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}, {7}
};

static int squared(int x) { return x * x; }

static void dpcm_lookahead(
        int& minimumError, size_t& minimumErrorIndex,
        const double *sampleBuf, const size_t lookahead, const int prevLevel)
{
    if (lookahead == 0) {
        minimumError = 0;
        return;
    }

    minimumError = std::numeric_limits<int>::max();
    minimumErrorIndex = dpcmLookupTable.size();
    int s = clamp(static_cast<int>(floor(sampleBuf[0] * 128.0)), -128, 127);
    std::vector<size_t> indexCandicateSet = dpcmIndexTable;
    if (dpcm_lookahead_fast) {
        indexCandicateSet = dpcmFastLookupTable[s - prevLevel + 255];
    }

    for (auto i : indexCandicateSet) {
        int newLevel = prevLevel + dpcmLookupTable[i];

        int recMinimumError;
        size_t recMinimumErrorIndex;

        // TODO apply dither noise
        int errorEstimation = squared(s - newLevel);
        if (errorEstimation >= minimumError)
            continue;

        dpcm_lookahead(recMinimumError, recMinimumErrorIndex,
                sampleBuf + 1, lookahead - 1, newLevel);

        // TODO weigh the error squared
        int error = squared(s - newLevel) + recMinimumError;
        if (error < minimumError) {
            if (newLevel <= 127 && newLevel >= -128) {
                minimumError = error;
                minimumErrorIndex = i;
            }
        }
    }
}

static double calculate_snr(const std::vector<double>& uncompressedData, const std::vector<int>& decompressedData)
{
    int sum_son = 0;
    int sum_mum = 0;

    assert(uncompressedData.size() == decompressedData.size());

    for(int i = 0; i < uncompressedData.size(); i++)
    {
        int s = clamp(static_cast<int>(floor(uncompressedData[i] * 128.0)), -128, 127) + 128;
        sum_son += s * s;
        int sub = decompressedData[i] + 128 - s;
        sum_mum += sub * sub;
    }

    if (sum_mum == 0)
    {
        return 100;
    }

    return 10 * std::log10((double)sum_son / sum_mum);
}

static void convert_dpcm(wav_file& wf, std::ofstream& ofs)
{
    int loop_sample = 0;

    uint32_t block_pos = 0;

    int minimumError;
    size_t minimumErrorIndex;

    std::vector<double> uncompressedData;
    std::vector<int> decompressedData;

    clock_t startTime,endTime;
    if (dpcm_verbose) {
        startTime = clock();
    }

    for (size_t i = 0; i <= wf.loopEnd; i += DPCM_BLK_SIZE) {
        double ds[DPCM_BLK_SIZE];
        wf.readData(i, ds, DPCM_BLK_SIZE);
        // check if loop end is inside this block
        if (i + DPCM_BLK_SIZE > wf.loopEnd) {
            assert(wf.loopEnd - i < DPCM_BLK_SIZE);
            ds[wf.loopEnd - i] = loop_sample;
        }
        if (dpcm_verbose) {
            uncompressedData.insert(uncompressedData.end(), std::begin(ds), std::end(ds));
        }
        // TODO apply dither noise
        int s = clamp(static_cast<int>(floor(ds[0] * 128.0)), -128, 127);

        if (wf.loopEnabled && i == wf.loopStart)
            loop_sample = s;

        data_write(ofs, block_pos, s, false);
        if (dpcm_verbose) {
            decompressedData.push_back(s);
        }

        size_t innerLoopCount = 1;
        uint8_t outData = 0;
        size_t sampleBufReadLen;

        goto initial_loop_enter;

        do {
            sampleBufReadLen = std::min(dpcm_enc_lookahead, DPCM_BLK_SIZE - innerLoopCount);
            dpcm_lookahead(
                    minimumError, minimumErrorIndex,
                    &ds[innerLoopCount], sampleBufReadLen, s);
            outData = static_cast<uint8_t>((minimumErrorIndex & 0xF) << 4);
            s += dpcmLookupTable[minimumErrorIndex];
            if (dpcm_verbose) {
                decompressedData.push_back(s);
            }
            innerLoopCount += 1;
initial_loop_enter:
            sampleBufReadLen = std::min(dpcm_enc_lookahead, DPCM_BLK_SIZE - innerLoopCount);
            dpcm_lookahead(
                    minimumError, minimumErrorIndex,
                    &ds[innerLoopCount], sampleBufReadLen, s);
            outData |= static_cast<uint8_t>(minimumErrorIndex & 0xF);
            s += dpcmLookupTable[minimumErrorIndex];
            innerLoopCount += 1;
            if (dpcm_verbose) {
                decompressedData.push_back(s);
            }
            data_write(ofs, block_pos, outData, true);
        } while (innerLoopCount < DPCM_BLK_SIZE);
    }

    if (dpcm_verbose) {
        endTime = clock();
        printf("SNR: %.2fdB, run time: %.2fs\n", calculate_snr(uncompressedData, decompressedData), (double)(endTime - startTime) / CLOCKS_PER_SEC);
    }
}

void enable_dpcm_verbose()
{
    dpcm_verbose = true;
}

void enable_dpcm_lookahead_fast()
{
    dpcm_lookahead_fast = true;
}

void set_dpcm_lookahead(size_t lookahead)
{
    dpcm_enc_lookahead = clamp<size_t>(lookahead, 1, 8);
}

void set_wav_loop_start(uint32_t start)
{
    wav_loop_start = start;
    wav_loop_start_override = true;
}

void set_wav_loop_end(uint32_t end)
{
    wav_loop_end = end;
    wav_loop_end_override = true;
}

void set_wav_tune(double tune)
{
    wav_tune = tune;
    wav_tune_override = true;
}

void set_wav_key(uint8_t key)
{
    wav_key = key;
    wav_key_override = true;
}

void set_wav_rate(uint32_t rate)
{
    wav_rate = rate;
    wav_rate_override = true;
}

void convert(const std::string& wav_file_str, const std::string& s_file_str,
        const std::string& sym, cmp_type ct)
{
    wav_file wf(wav_file_str);

    std::ofstream fout(s_file_str, std::ios::out);
    if (!fout.is_open()) {
        perror("ofstream");
        throw std::runtime_error("unable to open output file");
    }

    // check command line overrides
    if (wav_loop_start_override) {
        wf.loopStart = std::min(wav_loop_start, wf.loopEnd);
        wf.loopEnabled = true;
    }
    if (wav_loop_end_override) {
        wf.loopEnd = std::min(wav_loop_end, wf.loopEnd);
    }
    if (wav_tune_override) {
        wf.tuning = wav_tune;
    }
    if (wav_key_override) {
        wf.midiKey = wav_key;
    }
    if (wav_rate_override) {
        wf.sampleRate = wav_rate;
    }

    agb_out(fout, "    .section .rodata\n");
    agb_out(fout, "    .global %s\n", sym.c_str());
    agb_out(fout, "    .align  2\n\n%s:\n\n", sym.c_str());

    uint8_t fmt;
    if (ct == cmp_type::none)
        fmt = 0;
    else if (ct == cmp_type::dpcm)
        fmt = 1;
    else
        throw std::runtime_error("convert: invalid compression type");

    agb_out(fout, "    .byte   0x%X, 0x0, 0x0, 0x%X\n", fmt, wf.loopEnabled ? 0x40 : 0x0);
    double pitch;
    if (wf.midiKey == 60 && wf.tuning == 0.0)
        pitch = wf.sampleRate;
    else
        pitch = wf.sampleRate * pow(2.0, (60.0 - wf.midiKey) / 12.0 + wf.tuning / 1200.0);
    agb_out(fout, "    .word   0x%08X  @ Mid-C ~%f\n",
            static_cast<uint32_t>(pitch * 1024.0),
            pitch);
    agb_out(fout, "    .word   %u, %u\n", wf.loopStart, wf.loopEnd);

    if (ct == cmp_type::none)
        convert_uncompressed(wf, fout);
    else if (ct == cmp_type::dpcm)
        convert_dpcm(wf, fout);
    else
        throw std::runtime_error("convert: invalid compression type");

    agb_out(fout, "\n\n    .end\n");
}
