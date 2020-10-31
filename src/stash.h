
#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for asprintf()
#endif

#include <stdbool.h>

#include "list.h"

#include "stash_file.h"

typedef enum
{
  STASH_SUBCMD_SAVE,
  STASH_SUBCMD_POP
} stash_subcmd;

/** Initialize before any user input */
bool stash_init(void);

/** Initialize after user command line arguments */
bool stash_init_tmp(void);

bool stash_subcmd_lookup(const char* text, stash_subcmd* subcmd);

void stash_filename(const char* file, char* output);

bool stash_save(const char* file, const char* hunk_ids);

bool stash_pop(const char* text_file, const char* hunk_ids);

void stash_abort(const char* fmt, ...);
