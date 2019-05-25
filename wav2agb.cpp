#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cassert>

#include <string>

#include "converter.h"

static void usage() {
    fprintf(stderr, "wav2agb\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: wav2agb [options] <input.wav> [<output.s>]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "-s, --symbol <sym>       | symbol name for wave header (default: file name)\n");
    fprintf(stderr, "-l, --lookahead <amount> | DPCM compression lookahead 1..8 (default: 3)\n");
    fprintf(stderr, "-c, --compress           | compress output with DPCM\n");
    exit(1);
}

static void version() {
    printf("wav2agb v1.0 (c) 2019 ipatix\n");
    exit(0);
}

static void die(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(1);
}

static void fix_str(std::string& str) {
    // replaces all characters that are not alphanumerical
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] >= 'a' && str[i] <= 'z')
            continue;
        if (str[i] >= 'A' && str[i] <= 'Z')
            continue;
        if (str[i] >= '0' && str[i] <= '9' && i > 0)
            continue;
        str[i] = '_';
    }
}

static char path_seperators[] = {
    '/',
#ifdef _WIN32
    '\\',
#endif
    '\0'
};

static std::string filename_without_ext(const std::string& str) {
    size_t last_path_seperator = 0;
    char *sep = path_seperators;
    while (*sep) {
        size_t pos = str.find_last_of(*sep);
        if (pos != std::string::npos)
            last_path_seperator = std::max(pos, last_path_seperator);
        sep += 1;
    }
    size_t file_ext_dot_pos = str.find_last_of('.');
    if (file_ext_dot_pos == std::string::npos)
        return std::string(str);
    assert(file_ext_dot_pos != last_path_seperator);
    if (file_ext_dot_pos > last_path_seperator)
        return str.substr(0, file_ext_dot_pos);
    return std::string(str);
}

static std::string filename_without_dir(const std::string& str) {
    size_t last_path_seperator = 0;
    bool path_seperator_found = false;
    char *sep = path_seperators;
    while (*sep) {
        size_t pos = str.find_last_of(*sep);
        if (pos != std::string::npos) {
            last_path_seperator = std::max(pos, last_path_seperator);
            path_seperator_found = true;
        }
        sep += 1;
    }
    if (str.size() > 0 && path_seperator_found) {
        return str.substr(last_path_seperator + 1);
    } else {
        return std::string(str);
    }
}

static cmp_type arg_compress = cmp_type::none;
static std::string arg_sym;
static bool arg_input_file_read = false;
static bool arg_output_file_read = false;
static std::string arg_input_file;
static std::string arg_output_file;

int main(int argc, char *argv[]) {
    try {
        if (argc == 1)
            usage();

        for (int i = 1; i < argc; i++) {
            std::string st(argv[i]);
            if (st == "-s" || st == "--symbol") {
                if (++i >= argc)
                    die("-s: missing symbol name\n");
                arg_sym = argv[i];
                fix_str(arg_sym);
            } else if (st == "-c" || st == "--compress") {
                arg_compress = cmp_type::dpcm;
            } else if (st == "-l" || st == "--lookahead") {
                if (++i >= argc)
                    die("-l: missing parameter");
                set_dpcm_lookahead(std::stoul(argv[i], nullptr, 10));
            } else if (st == "--version") {
                version();
            } else {
                if (st == "--") {
                    if (++i >= argc)
                        die("--: missing file name\n");
                }
                if (!arg_input_file_read) {
                    arg_input_file = argv[i];
                    if (arg_input_file.size() < 1)
                        die("empty input file name\n");
                    arg_input_file_read = true;
                } else if (!arg_output_file_read) {
                    arg_output_file = argv[i];
                    if (arg_output_file.size() < 1)
                        die("empty output file name\n");
                    arg_output_file_read = true;
                } else {
                    die("Too many files specified\n");
                }
            }
        }

        // check arguments
        if (!arg_input_file_read) {
            die("No input file specified\n");
        }

        if (!arg_output_file_read) {
            // create output file name if none is provided
            arg_output_file = filename_without_ext(arg_input_file) + ".s";
            arg_output_file_read = true;
        }

        if (arg_sym.size() == 0) {
            arg_sym = filename_without_dir(filename_without_ext(arg_output_file));
            fix_str(arg_sym);
        }

        convert(arg_input_file, arg_output_file, arg_sym, arg_compress);
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "std lib error:\n%s\n", e.what());
    }
    return 1;
}
