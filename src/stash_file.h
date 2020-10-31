/*
 * stash_file.h
 *
 *  Created on: Sep 8, 2019
 *      Author: wozniak
 */

#pragma once

#include <stdbool.h>

#include "util.h"

typedef struct
{
  int fd;
  FILE* fp;
  char label[64];
  char name[path_max];
} stash_file;

void stash_file_init(stash_file* file, const char* label);

void stash_file_init_name(stash_file* file, const char* label,
                          const char* name);

bool stash_make_temp(stash_file* file, const char* label,
                     const char* template, int suffix_length);

bool stash_file_fopen_r(stash_file* file);

bool stash_file_fopen_w(stash_file* file);

bool stash_file_fopen_rw(stash_file* file);

bool stash_file_fopen_rw_exists(stash_file* file, bool* existed);

bool stash_file_fdopen(stash_file* file, const char* mode);

bool stash_file_clobber(stash_file* file);

bool stash_file_append(stash_file* file, const char* hunk);

bool stash_file_slurp(stash_file* file, char** result);

bool stash_file_close(stash_file* file);

bool stash_temp_delete(stash_file* file);
