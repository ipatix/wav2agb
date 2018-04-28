#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "converter.h"

static void usage() {
    fprintf(stderr, "usage:\n$ wav2agb <input.wav> <output.s> [-c]\n");
    exit(EXIT_FAILURE);
}

static void version() {
    printf("wav2agb v0.2 (c) 2018 by ipatix\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    //printf("wav2agb test version. Use at your own risk...\n");
    if (argc == 2) {
        if (strcmp(argv[1], "--version")) {
            usage();
        } else {
            version();
        }
    }
    if (argc < 3 || argc > 4)
        usage();
    bool comp = false;
    if (argc == 4) {
        if (strncmp(argv[3], "-c", 2)) {
            usage();
        } else {
            comp = true;
        }
    }

    const char *wav_file = argv[1];
    const char *s_file = argv[2];

    if (!file_exists(wav_file))
        die("Invalid input WAV file");

    convert(wav_file, s_file, comp);

    return EXIT_SUCCESS;
}
