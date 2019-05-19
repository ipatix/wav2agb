#include "converter.h"

#include <stdexcept>
#include <fstream>
#include <algorithm>

#include <cmath>
#include <cstdarg>
#include <cassert>
#include <cstring>

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

static void convert_uncompressed(wav_file& wf, std::ofstream& ofs)
{
    int loop_sample = 0;

    uint32_t block_pos = 0;

    for (size_t i = 0; i < wf.getLoopEnd(); i++) {
        double ds;
        wf.readData(i, &ds, 1);
        // TODO apply dither noise
        int s = std::clamp(static_cast<int>(floor(ds * 128.0)), -128, 127);

        if (wf.getLoopEnabled() && i == wf.getLoopStart())
            loop_sample = s;

        data_write(ofs, block_pos, s, false);
    }

    data_write(ofs, block_pos, loop_sample, false);
}

static size_t dpcm_enc_lookahead = 3;
static const size_t DPCM_BLK_SIZE = 0x40;
static const std::vector<int8_t> dpcmLookupTable = { 
    0, 1, 4, 9, 16, 25, 36, 49, -64, -49, -36, -25, -16, -9, -4, -1 
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

    for (size_t i = 0; i < dpcmLookupTable.size(); i++) {
        int newLevel = prevLevel + dpcmLookupTable[i];

        int recMinimumError;
        size_t recMinimumErrorIndex;

        dpcm_lookahead(recMinimumError, recMinimumErrorIndex,
                sampleBuf + 1, lookahead - 1, newLevel);

        // TODO apply dither noise
        int s = std::clamp(static_cast<int>(floor(sampleBuf[0] * 128.0)), -128, 127);

        // TODO weigh the error squared
        int error = squared(s - newLevel) + recMinimumError;
        if (error <= minimumError) {
            if (newLevel <= 127 && newLevel >= -128) {
                minimumError = error;
                minimumErrorIndex = i;
            }
        }
    }
}

static void convert_dpcm(wav_file& wf, std::ofstream& ofs)
{
    int loop_sample = 0;

    uint32_t block_pos = 0;

    int minimumError;
    size_t minimumErrorIndex;

    for (size_t i = 0; i <= wf.getLoopEnd(); i += DPCM_BLK_SIZE) {
        double ds[DPCM_BLK_SIZE];
        wf.readData(i, ds, DPCM_BLK_SIZE);
        // check if loop end is inside this block
        if (i + DPCM_BLK_SIZE > wf.getLoopEnd()) {
            assert(wf.getLoopEnd() - i < DPCM_BLK_SIZE);
            ds[wf.getLoopEnd() - i] = loop_sample;
        }
        // TODO apply dither noise
        int s = std::clamp(static_cast<int>(floor(ds[0] * 128.0)), -128, 127);

        if (wf.getLoopEnabled() && i == wf.getLoopStart())
            loop_sample = s;

        data_write(ofs, block_pos, s, false);

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
            innerLoopCount += 1;
initial_loop_enter:
            sampleBufReadLen = std::min(dpcm_enc_lookahead, DPCM_BLK_SIZE - innerLoopCount);
            dpcm_lookahead(
                    minimumError, minimumErrorIndex,
                    &ds[innerLoopCount], sampleBufReadLen, s);
            outData |= static_cast<uint8_t>(minimumErrorIndex & 0xF);
            s += dpcmLookupTable[minimumErrorIndex];
            innerLoopCount += 1;
            data_write(ofs, block_pos, outData, true);
        } while (innerLoopCount < DPCM_BLK_SIZE);
    }
}

void set_dpcm_lookahead(size_t lookahead)
{
    dpcm_enc_lookahead = std::clamp<size_t>(lookahead, 1, 8);
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

    agb_out(fout, "    .byte   0x%X, 0x0, 0x0, 0x%X\n", fmt, wf.getLoopEnabled() ? 0x40 : 0x0);
    double pitch;
    if (wf.getMidiKey() == 60 && wf.getTuning() == 0.0)
        pitch = wf.getSampleRate();
    else
        pitch = wf.getSampleRate() * pow(2.0, (60.0 - wf.getMidiKey()) / 12.0 + wf.getTuning() / 1200.0);
    agb_out(fout, "    .word   0x%08X  @ Mid-C ~%f\n",
            static_cast<uint32_t>(pitch * 1024.0),
            pitch);
    agb_out(fout, "    .word   0x%u, %u\n", wf.getLoopStart(), wf.getLoopEnd());

    if (ct == cmp_type::none)
        convert_uncompressed(wf, fout);
    else if (ct == cmp_type::dpcm)
        convert_dpcm(wf, fout);
    else
        throw std::runtime_error("convert: invalid compression type");

    agb_out(fout, "\n\n    .end\n");
}
