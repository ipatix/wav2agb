#pragma once

#include <string>
#include <algorithm>

enum class cmp_type {
    none, dpcm
};

void set_dpcm_lookahead(size_t lookahead);
void set_wav_loop_start(uint32_t start);
void set_wav_loop_end(uint32_t end);
void set_wav_tune(double tune);
void set_wav_key(uint8_t key);
void set_wav_rate(uint32_t rate);

void convert(const std::string&, const std::string&,
        const std::string& sym, cmp_type ct);
