
/*
 * stash_log.c
 *
 *  Created on: Sep 8, 2019
 *      Author: wozniak
 */

#include "stash_log.h"

#include <stdarg.h>
#include <stdio.h>

#include "util.h"
#include "stash_file.h"

stash_log_level stash_verbosity;

static const char*
stash_level_string(stash_log_level level)
{
  const char* result = "UNKNOWN";

  if (stash_verbosity == STASH_INFO &&
      level           == STASH_INFO)
    return "";

  switch (level)
  {
    case STASH_NULL:  result = "NULL  "; break;
    case STASH_TRACE: result = "TRACE "; break;
    case STASH_DEBUG: result = "DEBUG "; break;
    case STASH_INFO:  result = "INFO  "; break;
    case STASH_WARN:  result = "WARN  "; break;
    case STASH_ERROR: result = "ERROR "; break;
  }

  return result;
}

void
stash_log(stash_log_level level, const char* format, ...)
{
  if (level > stash_verbosity) return;

  char buffer[path_max*3];
  char* p = &buffer[0];
  int   length = path_max*3;
  va_list ap;
  va_start(ap, format);
  const char* token = stash_level_string(level);
  appendn (p, length, "stash: ");
  appendn (p, length, "%s", token);
  appendvn(p, length, format, ap);
  va_end(ap);
  printf("%s\n", buffer);
  fflush(stdout);
}
