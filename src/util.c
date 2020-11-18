
/**
   UTIL C
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> // just for sleep()

#include "util.h"

#if VALGRIND
#include <valgrind/valgrind.h>
static bool valgrind_linked = true;
#else
static bool valgrind_linked = false;
#endif

bool
file_size(const char* filename, size_t* length)
{
  struct stat s;
  int rc = stat(filename, &s);
  if (rc != 0)
  {
    printf("file_size(): could not stat: %s\n", filename);
    return false;
  }

  *length = s.st_size;
  return true;
}

bool
file_size_fp(FILE* fp, size_t* length)
{
  struct stat s;
  int rc = fstat(fileno(fp), &s);
  if (rc != 0)
  {
    printf("file_size_fp(): could not fstat!\n");
    return false;
  }

  *length = s.st_size;
  return true;
}

char*
slurp(const char* filename)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("slurp(): could not read from: %s\n", filename);
    return NULL;
  }

  char* result = slurp_fp(filename, fp);

  fclose(fp);
  return result;
}

char*
slurp_fp(const char* filename, FILE* fp)
{
  size_t length;
  bool b = file_size_fp(fp, &length);
  if (!b)
  {
    printf("slurp_fp(): could not stat: %s\n", filename);
    return NULL;
  }

  char* result = malloc(length+1);
  if (result == NULL)
  {
    printf("slurp(): could not allocate memory for: %s\n", filename);
    return NULL;
  }

  char* p = result;
  size_t actual = fread(p, sizeof(char), length, fp);
  if (actual != length)
  {
    printf("could not read all %zi bytes from file: %s\n",
           length, filename);
    free(result);
    return NULL;
  }
  result[length] = '\0';
  return result;
}

/**
   Is there another way to detect we are under valgrind?
 */
static bool
using_valgrind(void)
{
  if (! valgrind_linked) return false;
  char* s = getenv("LD_PRELOAD");
  if (s == NULL || strlen(s) == 0)
    return false;
  char* m = strstr(s, "vgpreload");
  if (m == NULL)
    return false;
  return true;
}

void
valgrind_assert_failed(const char* file, int line)
{
  fprintf(stderr, "valgrind_assert(): failed: %s:%d\n", file, line);
  if (using_valgrind())
  {
    // This will give us more information from valgrind
    #if VALGRIND
    fprintf(stderr, "valgrind_assert(): dumping stack...\n");
    VALGRIND_PRINTF_BACKTRACE("%s:%d", file, line);
    #endif
  }
  printf("valgrind_assert(): calling exit(EXIT_FAILURE)\n");
  exit(EXIT_FAILURE);
}

bool
getenv_string(const char* key, char** value)
{
  char* t = getenv(key);
  if (t == NULL)
    return false;
  if (*t == '\0')
    return false;
  *value = t;
  return true;
}

bool
mkdirp(const char* path)
{
  char work[path_max];
  char done[path_max];
  strcpy(work, path);

  char* p = strtok(work, "/");
  done[0] = '/';
  char* q = &done[1];
  q = stpcpy(q, p);

  mode_t mode = S_IRWXU;
  while (true)
  {
    int rc = mkdir(done, mode);
    if (rc != 0 && errno != EEXIST)
    {
      printf("could not mkdir: '%s'\n", done);
      perror("stash");
      return false;
    }

    if (p == NULL) break;
    p = strtok(NULL, "/");
    if (p != NULL)
    {
      *q = '/';
      q++;
      q = stpcpy(q, p);
    }
  }
  return true;
}

bool
vprintf_color(const char* color, const char* fmt, va_list ap)
{
  int rc;
  rc = printf("%s", color);
  CHECK(rc > 0, "error in vprintf_color!");
  rc = vprintf(fmt, ap);
  CHECK(rc > 0, "error in vprintf_color!");
  rc = printf(RESET);
  CHECK(rc > 0, "error in vprintf_color!");
  return true;
}

bool
printf_color(const char* color, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  bool result = vprintf_color(color, fmt, ap);

  va_end(ap);
  return result;
}
