
/*
 * stash_file.c
 *
 *  Created on: Sep 8, 2019
 *      Author: wozniak
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stash_file.h"
#include "stash_log.h"

static inline void
stash_file_reset(stash_file* file)
{
  file->fd = -100;
  file->fp = NULL;
}

void
stash_file_init(stash_file* file, const char* label)
{
  strcpy(file->label, label);
  stash_file_reset(file);
}

void
stash_file_init_name(stash_file* file,
                     const char* label, const char* name)
{
  stash_file_init(file, label);
  strcpy(file->name, name);
}

bool
stash_make_temp(stash_file* file, const char* label,
                const char* template, int suffix_length)
{
  stash_file_init(file, label);
  strcpy(file->name, template);
  int fd = mkstemps(file->name, suffix_length);
  CHECK(fd != -1, "could not mkstemps: %s", file->name);
  file->fd = fd;
  return true;
}

bool
stash_file_fopen_r(stash_file* file)
{
  file->fp = fopen(file->name, "r");
  CHECK(file->fp != NULL, "could not fopen (r/o): %s", file->name);
  return true;
}

bool
stash_file_fopen_w(stash_file* file)
{
  file->fp = fopen(file->name, "w");
  CHECK(file->fp != NULL, "could not fopen (w/o): %s", file->name);
  return true;
}

bool
stash_file_fopen_rw(stash_file* file)
{
  stash_log(STASH_DEBUG, "file open r/w: [%s] %s",
            file->label, file->name);
  mode_t mode = S_IRUSR | S_IWUSR;
  file->fd = open(file->name, O_RDWR|O_CREAT, mode);
  CHECK(file->fd > 0, "could not open (r/w): '%s': %s",
        file->name, strerror(errno));
  file->fp = fdopen(file->fd, "r+");
  CHECK(file->fp != NULL, "could not fdopen (r/w): %s", file->name);
  return true;
}

bool
stash_file_fopen_rw_exists(stash_file* file, bool* existed)
{
  file->fp = fopen(file->name, "r+");
  if (file->fp == NULL)
  {
    *existed = false;
    stash_log(STASH_DEBUG,
              "there is no existing file at %s", file->name);
    file->fp = fopen(file->name, "w");
    CHECK(file->fp != NULL, "could not create: %s", file->name);
  }
  else
    *existed = true;
  return true;
}

bool
stash_file_fdopen(stash_file* file, const char* mode)
{
  file->fp = fdopen(file->fd, mode);
  CHECK(file->fp != NULL, "stash_file_fdopen(): failed!");
  return true;
}

bool
stash_file_append(stash_file* file, const char* hunk)
{
  stash_log(STASH_TRACE, "stash_file_append: [%s]", file->label);
  int count = fprintf(file->fp, "%s", hunk);
  CHECK(count >= 0, "could not write to: %s", file->name);
  fflush(file->fp);
  return true;
}

bool
stash_file_slurp(stash_file* file, char** result)
{
  if (file->fp == NULL)
    stash_file_fopen_r(file);

  *result = slurp_fp(file->name, file->fp);
  CHECK(*result != NULL, "stash_slurp() failed: %s", file->name);
  return true;
}

bool
stash_file_clobber(stash_file* file)
{
  assert(file->fp != NULL);
  clearerr(file->fp);
  int rc;
  rc = fseek(file->fp, 0, SEEK_SET);
  assert(rc == 0);
  rc = ftruncate(fileno(file->fp), 0);
  assert(rc == 0);
  return true;
}

bool
stash_file_close(stash_file* file)
{
  if (file->fp != NULL)
    fclose(file->fp);
  else if (file->fd > 0)
    close(file->fd);
  else
    FAIL("stash_file_close(): not open: %s", file->name);

  stash_file_reset(file);

  return true;
}

bool
stash_temp_delete(stash_file* file)
{
  bool b = stash_file_close(file);
  CHECK(b, "stash_temp_delete(): could not close: %s", file->name);
  unlink(file->name);
  return true;
}
