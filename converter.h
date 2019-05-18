#pragma once

#include <string>

enum class cmp_type {
    none, dpcm
};

void set_dpcm_lookahead(size_t lookahead);
void convert(const std::string&, const std::string&,
        const std::string& sym, cmp_type ct);
