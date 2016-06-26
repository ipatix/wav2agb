#pragma once

#include <sndfile.h>

bool file_exists(const char file_path[]);
void die(const char msg[]);
void pdie(const char msg[]);
void snddie(SNDFILE *);
void strip_exitension(char filename[], const char ext[]);
