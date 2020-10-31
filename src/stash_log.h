/*
 * stash_log.h
 *
 *  Created on: Sep 8, 2019
 *      Author: wozniak
 */

#pragma once

typedef enum
{
  STASH_NULL  = 0,
  STASH_TRACE = 5,
  STASH_DEBUG = 4,
  STASH_INFO  = 3,
  STASH_WARN  = 2,
  STASH_ERROR = 1,
} stash_log_level;

extern stash_log_level stash_verbosity;

void stash_log(stash_log_level level, const char* format, ...);
