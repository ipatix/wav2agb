#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

bool file_exists(const char file_path[]) {
    struct stat file_info;
    if (stat(file_path, &file_info)) {
        return false;
    }
    if (S_ISREG(file_info.st_mode)) 
        return true;
    else 
        return false;
}

void die(const char msg[]) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void pdie(const char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void snddie(SNDFILE *file) {
    fprintf(stderr, "libsndfile: %s\n", sf_strerror(file));
    exit(EXIT_FAILURE);
}

void strip_extension(char filename, const char ext[]) {
    if (strlen(filename) < strlen(ext) + 1)
        return;
    char *ext_in_name = &filename[strlen(filename) - strlen(ext) - 1];
    if (strcmp(ext_in_name+1, ext))
        return;
    while(*ext_in_name)
        *ext_in_name++ = '\0';
}
