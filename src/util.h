
/**
   UTIL H
*/

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/** Used for filenames */
#define path_max 4096

#define CHECK(condition, format, args...)         \
  do {                                            \
    if (!(condition)) {                           \
      FAIL(format, ## args);                      \
    }                                             \
  } while (0);

#define FAIL(format, args...)                   \
  do {                                          \
    printf("stash: " format "\n", ## args);     \
    return false;                               \
  } while (0);

#define CHECK_GOTO(condition, label, format, args...)   \
  do {                                                  \
    if (!(condition)) {                                 \
      FAIL_GOTO(label, format, ## args);                \
    }                                                   \
  } while (0);

#define FAIL_GOTO(label, format, args...)       \
  do {                                          \
    printf("stash: " format "\n", ## args);     \
    result = false;                             \
    goto label;                                 \
  } while (0);

bool file_size(const char* filename, size_t* length);

char* slurp(const char* filename);

/** The filename is just for debugging */
char* slurp_fp(const char* filename, FILE* fp);

#define append(string,  args...) \
  string += sprintf(string, ## args)

#define appendn(string, n, args...)             \
  do {                                          \
    int _c = snprintf(string, n, ## args);      \
    assert(_c <= n);                            \
    string += _c;                               \
    n      -= _c;                               \
  } while(0);

// untested
#define appendv(string, format, ap)             \
  string += vsprintf(string, format, ap)

// untested
#define appendvn(string, n, format, ap)         \
  do {                                          \
    int _c = vsnprintf(string, n, format, ap);  \
    assert(_c <= n);                            \
    string += _c;                               \
    n -= _c;                                    \
  } while(0);

__attribute__ ((unused))
static char*
plural(int count)
{
  if (count == 1) return "";
  return "s";
}

/** Called when the valgrind_assert() condition fails */
void valgrind_assert_failed(const char *file, int line);

/** Called when the valgrind_assert_msg() condition fails */
void valgrind_assert_failed_msg(const char *file, int line,
                                const char* format, ...);

/**
   VALGRIND_ASSERT
   Substitute for assert(): provide stack trace via valgrind
   If not running under valgrind, works like assert()
 */
#ifdef NDEBUG
#define valgrind_assert(condition)            (void) (condition);
#define valgrind_assert_msg(condition,msg...) (void) (condition);
#else
#define valgrind_assert(condition) \
    if (!(condition)) \
    { valgrind_assert_failed(__FILE__, __LINE__); }
#define valgrind_assert_msg(condition,msg...) \
    if (!(condition)) \
    { valgrind_assert_failed_msg(__FILE__, __LINE__, ## msg); }
#endif

/**
   Cause valgrind assertion error behavior w/o condition
 */
#define valgrind_fail(msg...) \
  valgrind_assert_failed_msg(__FILE__, __LINE__, ## msg)

bool getenv_string(const char* key, char** value);

bool mkdirp(const char* path);

#define RESET       "\033[0m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define BOLDBLACK   "\033[1m\033[30m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGREEN   "\033[1m\033[32m"
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\033[1m\033[34m"

bool printf_color(const char* color, const char* fmt, ...);
bool vprintf_color(const char* color, const char* fmt, va_list ap);
