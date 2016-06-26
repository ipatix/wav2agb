#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "converter.h"

static void usage() {
    fprintf(stderr, "usage:\n$ wav2agb <input.wav> <output.s> [-s]\n");
    exit(EXIT_FAILURE);
}

static void version() {
    printf("wav2agb v0.1 (c) 2016 by ipatix\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (!strncmp(argv[1], "--version", 9)) {
            usage();
        } else {
            version();
        }
    }
    if (argc < 3 || argc > 4)
        usage();
    bool comp = false;
    if (argc == 4) {
        if (!strncmp(argv[3], "-c", 2)) {
            usage();
        } else {
            comp = true;
        }
    }

    const char *wav_file = argv[1];
    const char *s_file = argv[2];

    if (!file_exists(wav_file))
        die("Invalid input WAV file");
    if (!file_exists(s_file))
        die(stderr, "Invalid output S file.");

    if (comp)
        convert_compressed(wav_file, s_file);
    else
        convert_normal(wav_file, s_file);

    return EXIT_SUCCESS;
}
