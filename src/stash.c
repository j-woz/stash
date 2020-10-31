
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "buffer.h"
#include "stash.h"
#include "stash_log.h"
#include "util.h"

/** Used for reading lines of text */
#define MAX_LINE 1024

/** Used for temp files */
static char tmp_name_template[path_max];
static int  tmp_name_suffix_length;

bool
stash_init()
{
  stash_verbosity = STASH_INFO;
  return true;
}

bool
stash_init_tmp()
{
  char  buffer[path_max];
  char* tmpdir;
  if (! (getenv_string("STASH_TMP", &tmpdir) ||
         getenv_string("TMPDIR", &tmpdir)))
  {
    char* user = getenv("USER");
    sprintf(buffer, "/tmp/%s/stash", user);
    tmpdir = &buffer[0];
  }
  stash_log(STASH_DEBUG, "tmpdir: %s", tmpdir);
  bool b = mkdirp(tmpdir);
  CHECK(b, "could not make tmpdir: %s", tmpdir);
  char* suffix = ".txt";
  tmp_name_suffix_length = strlen(suffix);
  sprintf(tmp_name_template, "%s/stash.XXXXXX%s", tmpdir, suffix);
  return true;
}

static void
stash_temp_file(stash_file* file, const char* label)
{
  stash_make_temp(file, label,
                  tmp_name_template, tmp_name_suffix_length);
  stash_log(STASH_DEBUG, "temp file: [%s] %s", label, file->name);
}

static void
stash_temp_file_fopen(stash_file* file, const char* label)
{
  stash_temp_file(file, label);
  stash_file_fdopen(file, "r+");
}

/*
static void
cat(const char* filename)
{
  char cmd[path_max*3];
  sprintf(cmd, "cat %s", filename);
  stash_log(STASH_INFO, cmd);
  system(cmd);
}
*/

bool
stash_subcmd_lookup(const char* text, stash_subcmd* subcmd)
{
  if (text[0] == 's')
    *subcmd = STASH_SUBCMD_SAVE;
  else if (text[0] == 'p')
    *subcmd = STASH_SUBCMD_POP;
  else
    return false;
  return true;
}

static bool hunk_ids_contains(int index, struct list* hunk_ids);
static bool stash_save_hunks(struct list* hunks,
                             struct list* hunk_ids,
                             const char* stash_name);
static bool stash_resolve(struct list* hunks, struct list* hunk_ids,
                          const char* text_name);

bool stash_parse_diff(stash_file* diff, struct list* hunks);
bool stash_make_diff(const char* file, stash_file* diff);

static bool stash_save_hunks_interactive(struct list* hunks,
                                         const char* text_name,
                                         stash_file* diff);

bool stash_save_ids(struct list* hunks, const char* hunk_ids_s,
                    const char* text_name, const char* stash_name);

bool
stash_save(const char* text_name, const char* hunk_ids_s)
{
  bool result = true;
  CHECK(text_name != NULL, "provide a file!");

  stash_file diff;
  stash_temp_file_fopen(&diff, "diff");

  bool b;
  b = stash_make_diff(text_name, &diff);
  CHECK_GOTO(b, done2, "stash save: could not make diff");

  stash_file stash;
  stash_file_init(&stash, "stash");
  stash_filename(text_name, stash.name);
  stash_log(STASH_DEBUG, "the stash file is: %s", stash.name);
  struct list hunks;
  list_init(&hunks);
  stash_parse_diff(&diff, &hunks);
  if (hunks.size == 0)
  {
    stash_log(STASH_INFO, "no changes in %s.", text_name);
    goto done1;
  }
  stash_log(STASH_INFO, "found %i hunk%s",
            hunks.size, plural(hunks.size));
  if (hunk_ids_s == NULL)
    b = stash_save_hunks_interactive(&hunks, text_name, &diff);
  else
    b = stash_save_ids(&hunks, hunk_ids_s, text_name, stash.name);

  CHECK_GOTO(b, done1, "save failed!");

  done1:
  list_destruct(&hunks, NULL);
  done2:
  stash_temp_delete(&diff);
  return result;
}

static bool stash_pop_hunks(struct list* hunks, const char* text_name,
                            stash_file* hunk_file, bool* modified);

static bool stash_pop_hunks_interactive(struct list* hunks,
                                        const char* text_name,
                                        stash_file* hunk_file,
                                        bool* modified);

static bool stash_overwrite_stash(struct list* hunks,
                                  const char* stash_name);

bool
stash_pop(const char* text_name, const char* hunk_ids_s)
{
  struct list hunks;
  list_init(&hunks);

  bool b;
  stash_file stash;
  stash_file_init(&stash, "stash");
  stash_filename(text_name, stash.name);
  b = stash_file_fopen_r(&stash);
  CHECK(b, "pop: could not open stash: %s", stash.name);
  stash_parse_diff(&stash, &hunks);
  stash_file_close(&stash);

  if (hunks.size == 0)
  {
    stash_log(STASH_INFO, "no hunks!");
    return true;
  }
  stash_log(STASH_INFO, "hunks: %i\n", hunks.size);

  stash_file hunk;
  stash_temp_file_fopen(&hunk, "hunk");
  stash_log(STASH_DEBUG, "hunk file name: %s\n", hunk.name);

  bool modified;
  if (hunk_ids_s == NULL)
    stash_pop_hunks_interactive(&hunks, text_name, &hunk, &modified);
  else
    stash_pop_hunks(&hunks, text_name, &hunk, &modified);

  if (modified) stash_overwrite_stash(&hunks, stash.name);

  list_destruct(&hunks, NULL);
  return true;
}

static bool stash_pop_hunk(const char* text_name, const char* hunk,
                           stash_file* hunk_file);

static bool
stash_pop_hunks(struct list* hunks, const char* text_name,
                stash_file* hunk_file, bool* modified)
{
  for (int i = 0; i < hunks->size; i++)
  {
    void* v; // Needed to avoid pointer aliasing
    list_get(hunks, i, &v);
    char* hunk = v;
    bool b;
    b = stash_pop_hunk(text_name, hunk, hunk_file);
    CHECK(b, "could not pop hunk!");
    b = list_remove(hunks, v);
    assert(b);
    *modified = true;
  }
  return true;
}

static bool string2file(const char* filename, FILE* fp,
                        const char* s);

static int
get1char(void)
{
  int c = getc(stdin);
  if (c != '\n') getc(stdin); // The newline
  return c;
}

static bool
stash_pop_hunks_interactive(struct list* hunks, const char* text_name,
                            stash_file* hunk_file, bool* modified)
{
  int  index  = 0;
  bool loop   = true;
  bool result = true;
  bool b;
  *modified = false;
  while (loop)
  {
    void* v; // Needed to avoid pointer aliasing
    list_get(hunks, index, &v);
    char* hunk = v;
    printf_color(BLUE, "hunk %i:\n", index);
    printf("%s\n", hunk);
    printf_color(BLUE, "[p]op [d]rop s[k]ip [q]uit: ");
    int c = get1char();
    switch (c)
    {
      case 'p':
        b = stash_pop_hunk(text_name, hunk, hunk_file);
        if (index >= hunks->size) index = hunks->size - 1;
        if (b)
        {
          *modified = true;
          list_remove(hunks, hunk);
          free(hunk);
        }
        else
        {
          result = false;
          loop   = false;
        }
        break;
      case 'd':
        *modified = true;
        list_remove(hunks, hunk);
        free(hunk);
        if (index >= hunks->size) index = hunks->size - 1;
        break;
      case 'k':
        index++; // Noop
        break;
      case 'q':
        loop = false;
        break;
      // TODO: handle others!
    }
    if (index < 0 || index >= hunks->size)
    {
      stash_log(STASH_INFO, "no more hunks.");
      break;
    }
  }
  return result;
}

static bool
stash_pop_hunk(const char* text_name, const char* hunk,
               stash_file* hunk_file)
{
  bool b = string2file(hunk_file->name, hunk_file->fp, hunk);
  CHECK(b, "could not write hunk to: %s", hunk_file->name);

  char cmd[path_max*3];
  sprintf(cmd, "patch %s %s", text_name, hunk_file->name);
  stash_log(STASH_INFO, "patching %s ...", text_name);
  stash_log(STASH_DEBUG, "cmd: %s\n", cmd);

  int rc = system(cmd);
  CHECK(rc == 0, "could not patch: %s", text_name);

  stash_log(STASH_INFO, "patched %s.", text_name);
  return true;
}

/** TODO: Move to stash_file API */
static bool
string2file(const char* filename, FILE* fp, const char* s)
{
  stash_log(STASH_DEBUG, "string2file: writing to: %s", filename);

  int rc;
  rc = fseek(fp, 0, SEEK_SET);
  if (rc != 0) return false;

  rc = ftruncate(fileno(fp), 0);
  if (rc != 0) return false;

  size_t length = strlen(s);
  rc = fwrite(s, sizeof(*s), length, fp);
  if (rc != length)
  {
    perror("stash");
    return false;
  }
  if (ferror(fp)) return false;
  fflush(fp);

  stash_log(STASH_DEBUG, "string2file: OK");
  return true;
}

void
stash_filename(const char* filename, char* output)
{
  int count = sprintf(output, "%s.stash", filename);
  if (count < 0)
    stash_abort("Could not create stash filename for: %s", filename);
}

void
stash_filename_backup(const char* filename, char* output)
{
  int count = sprintf(output, "%s.stash~", filename);
  if (count < 0)
    stash_abort("Could not create stash backup filename for: %s",
                filename);
}

bool
stash_make_diff(const char* file, stash_file* diff)
{
  char cmd_short[path_max+128]; // For error message
  char cmd[path_max*4];

  sprintf(cmd_short, "svn diff %s", file);
  sprintf(cmd, "%s > %s", cmd_short, diff->name);

  stash_log(STASH_DEBUG, "running: %s", cmd_short);
  int rc = system(cmd);
  CHECK(rc == 0, "error occurred in command: %s", cmd_short);

  rewind(diff->fp);

  return true;
}

typedef enum
{
  READ_MORE,  // The result is good data
  READ_END,   // The result is EOF
  READ_ERROR  // An error occurred
} read_result;

typedef enum
{
  HUNK_PROTO, // New variable
  HUNK_MORE,  // The next line is a hunk "@@"
  HUNK_END,   // The next line is the end of file
  HUNK_ERROR  // An error occurred
} hunk_result;

static hunk_result   read_hunk(FILE* diff, char* line, int* number,
                             buffer* b);
static read_result read_line(FILE* fp, char* line, int number);

/** adds hunks in diff_name/diff_fp to list hunks */
bool
stash_parse_diff(stash_file* diff, struct list* hunks)
{
  int number = 1;
  stash_log(STASH_DEBUG, "parsing diff: %s", diff->name);
  // Reusable line buffer:
  char line[MAX_LINE];
  while (true)
  {
    read_result rr = read_line(diff->fp, line, number);
    CHECK(rr != READ_ERROR, "read error in %s", diff->name);
    if (rr == READ_END) return true;
    if (strncmp(line, "Index:", 6) == 0) continue;
    if (strncmp(line, "+++",    3) == 0) continue;
    if (strncmp(line, "---",    3) == 0) continue;
    if (strncmp(line, "===",    3) == 0) continue;
    break; // found some text
    number++;
  }
  buffer B;
  buffer_init(&B, MAX_LINE);
  hunk_result hr = HUNK_PROTO;
  while (true)
  {
    if (strncmp(line, "@@", 2) == 0)
    {
      buffer_appendv(&B, "%s", line);
      hr = read_hunk(diff->fp, line, &number, &B);
      CHECK(hr != HUNK_ERROR, "read error in %s", diff->name);
      char* hunk = buffer_dup(&B);
      list_add(hunks, hunk);
    }
    if (hr == HUNK_END) break;
    buffer_reset(&B);
  }
  buffer_finalize(&B);
  return true;
}

static read_result
read_line(FILE* fp, char* line, int number)
{
  if (fgets(line, MAX_LINE, fp) == NULL)
    return READ_END;
  char* c = strchr(line, '\n');
  if (c == NULL)
  {
    printf("stash: line %i is too long! limit: %i\n",
           number, MAX_LINE);
    return READ_ERROR;
  }
  return READ_MORE;
}

/**
   diff_fp: IN:     The file pointer to use
   line:    IN:     The current line buffer (reusable)
   number:  IN/OUT: The current line number
   buffer:     OUT: New buffer contents are appended here
 */
static hunk_result
read_hunk(FILE* diff_fp, char* line, int* number, buffer* b)
{
  while (true)
  {
    if (fgets(line, MAX_LINE, diff_fp) == NULL)
      break;
    char* c = strchr(line, '\n');
    if (c == NULL)
    {
      printf("stash: line %i is too long! limit: %i\n",
             *number, MAX_LINE);
      return HUNK_ERROR;
    }
    (*number)++;
    if (strncmp(line, "@@", 2) == 0)
      return HUNK_MORE;
    bool rc = buffer_append(b, line);
    if (!rc)
      stash_abort("Failed to allocate memory!");
  }
  return HUNK_END;
}

static bool
hunk_ids_contains(int index, struct list* hunk_ids)
{
  for (struct list_item* item = hunk_ids->head;
       item != NULL;
       item = item->next)
  {
    char* spec = item->data;
    stash_log(STASH_TRACE,
              "hunk_ids_contains: index=%i spec=%s", index, spec);
    if (strcmp(spec, "@") == 0)
      return true;
    // TODO: Handle ranges
    errno = 0;
    char* end;
    long value = strtol(spec, &end, 10);
    if (end == spec || errno != 0)
      stash_abort("bad integer in hunk spec: '%s'", spec);
    // printf("value=%li spec=%s\n", value, spec);
    if (value == index)
      // Found:
      return true;
  }

  // Not found:
  return false;
}

static bool stash_resolve_hunk(const char* hunk,
                               const char* text_name,
                               stash_file* diff,
                               const char* errs_name);

static bool cp_fps(FILE* fp1, FILE* fp2);

static int prompt_save(int index, const char* hunk);

static void save_i_switch_s(bool success, int* index, int* saves,
                            bool* loop, bool* result);

static void save_i_switch_d(bool success, int* index,
                            bool* loop, bool* result);

static bool
stash_save_hunks_interactive(struct list* hunks,
                             const char*  text_name,
                             stash_file*  diff)
{
  stash_log(STASH_DEBUG, "stash_save_hunks_interactive...");

  stash_file stash, stash_backup;
  bool stash_existed;
  char* stash_previous = NULL;
  stash_file_init(&stash,        "stash");
  stash_file_init(&stash_backup, "backup");
  stash_filename       (text_name, stash.name);
  stash_filename_backup(text_name, stash_backup.name);
  stash_file_fopen_rw_exists(&stash, &stash_existed);

  if (stash_existed)
  {
    stash_file_fopen_w(&stash_backup);
    stash_file_slurp(&stash, &stash_previous);
    stash_file_append(&stash_backup, stash_previous);
    stash_file_close(&stash_backup);
    stash_file_clobber(&stash);
  }

  stash_file errs;
  stash_temp_file(&errs, "errors");

  int  index  = 0;
  int  saves  = 0;
  bool loop   = true;
  bool result = true;
  bool b;

  while (loop)
  {
    void* v;
    list_get(hunks, index, &v);
    char* hunk = v;
    int c = prompt_save(index, hunk);
    switch (c)
    {
      case 's':
        stash_file_append(&stash, hunk);
        b = stash_resolve_hunk(hunk, text_name, diff, errs.name);
        save_i_switch_s(b, &index, &saves, &loop, &result);
        break;
      case 'd':
        b = stash_resolve_hunk(hunk, text_name,
                               diff, errs.name);
        save_i_switch_d(b, &index, &loop, &result);
        break;
      case 'k':
        index++; // Noop
        break;
      case 'q':
        loop = false;
        break;
    }
    if (index >= hunks->size)
      break;
  }
  stash_temp_delete(&errs);

  if (stash_existed)
  {
    stash_file_append(&stash, stash_previous);
    free(stash_previous);
  }

  stash_file_close(&stash);

  stash_log(STASH_INFO, "saved %i hunk%s to %s",
            saves, plural(saves), stash.name);
  return result;
}

static int
prompt_save(int index, const char* hunk)
{
  printf_color(BLUE, "hunk");
  printf(" %i", index+1);
  printf_color(BLUE, ":");
  printf("\n");
  printf("%s\n", hunk);
  printf_color(BLUE, "[s]ave [d]rop s[k]ip [q]uit: ");
  int c = get1char();
  return c;
}

/**
   save-interactive-switch-s : s means save this hunk
   @param success: are we in a successful state?
 */
static void
save_i_switch_s(bool success, int* index, int* saves,
                bool* loop, bool* result)
{
  if (success)
  {
    (*saves)++;
  }
  else
  {
    *result = false;
    *loop   = false;
  }

  (*index)++;
}

/**
   save-interactive-switch-d : d means drop this hunk
   @param success: are we in a successful state?
 */
static void
save_i_switch_d(bool success, int* index, bool* loop, bool* result)
{
  if (!success)
  {
    *result = false;
    *loop   = false;
  }
  (*index)++;
}

bool
stash_save_ids(struct list* hunks, const char* hunk_ids_s,
               const char* text_name, const char* stash_name)
{
  bool b;
  struct list hunk_ids;
  list_init(&hunk_ids);
  list_split(&hunk_ids, hunk_ids_s, ',');
  b = stash_save_hunks(hunks, &hunk_ids, stash_name);
  CHECK(b, "save failed!");
  b = stash_resolve(hunks, &hunk_ids, text_name);
  CHECK(b, "resolve failed to %s!", text_name);
  list_destruct(&hunk_ids, NULL);
  return true;
}

static bool
cp_fps(FILE* fp1, FILE* fp2)
{
  stash_log(STASH_TRACE, "cp_fps() ...");
  const int chunk = 64*1024;
  char t[chunk];
  while (true)
  {
    stash_log(STASH_TRACE, "chunk:  %i %p", chunk, fp1);
    int actual = fread(t, 1, chunk, fp1);
    stash_log(STASH_TRACE, "actual:  %i", actual);
    int written = fwrite(t, 1, actual, fp2);
    stash_log(STASH_TRACE, "written: %i", written);
    CHECK(actual == written, "cp_fps write error!");
    if (written < chunk) break;
  }
  return true;
}

static bool
stash_save_hunks(struct list* hunks, struct list* hunk_ids,
                 const char* stash_name)
{
  stash_file tmp;
  stash_temp_file_fopen(&tmp, "save_tmp");

  struct list_item* item = hunks->head;
  int i = 1;
  while (item != NULL)
  {
    if (hunk_ids_contains(i, hunk_ids))
      stash_file_append(&tmp, item->data);
    item = item->next;
    i++;
  }

  stash_file stash;
  stash_file_init_name(&stash, "stash", stash_name);
  stash_file_fopen_rw(&stash);
  cp_fps(stash.fp, tmp.fp);
  // fflush(tmp.fp);
  rewind(tmp.fp);
  rewind(stash.fp);
  cp_fps(tmp.fp, stash.fp);

  bool b;
  b = stash_file_close(&stash);
  CHECK(b, "close error");
  b = stash_file_close(&tmp);
  CHECK(b, "close error");
  return true;
}

static bool
stash_overwrite_stash(struct list* hunks, const char* stash_name)
{
  // TODO: Make backup file
  stash_log(STASH_INFO, "overwriting %s with %i hunks.",
            stash_name, hunks->size);
  bool b;
  stash_file stash;
  stash_file_init_name(&stash, "stash", stash_name);
  b = stash_file_fopen_w(&stash);
  CHECK(b, "could not overwrite stash!");
  for (struct list_item* item = hunks->head;
       item != NULL;
       item = item->next)
  {
    b = stash_file_append(&stash, item->data);
    CHECK(b, "write failed!");
  }
  b = stash_file_close(&stash);
  assert(b);
  return true;
}

static bool
stash_resolve(struct list* hunks, struct list* hunk_ids,
              const char* text_name)
{
  stash_log(STASH_DEBUG, "resolving: %s", text_name);
  stash_file diff, errs;
  stash_temp_file_fopen(&diff, "diff");
  stash_temp_file(&errs, "errs");

  struct list_item* item = hunks->head;
  int i = 1;
  while (item != NULL)
  {
    if (hunk_ids_contains(i, hunk_ids))
      stash_resolve_hunk(item->data, text_name,
                         &diff, errs.name);
    item = item->next;
    i++;
  }

  stash_file_close(&errs);

  return true;
}

static bool patch_errs(int rc, const char* errs_name);

static bool
stash_resolve_hunk(const char* hunk, const char* text_name,
                   stash_file* diff,
                   const char* errs_name)
{
  stash_log(STASH_DEBUG, "stash_resolve_hunk...");
  stash_file_clobber(diff);
  size_t length = strlen(hunk);

  int rc;
  rc = fwrite(hunk, sizeof(char), length, diff->fp);
  CHECK(rc >= 0, "could not write hunk to %s", diff->name);
  fflush(diff->fp);

  char cmd[path_max*3];
  sprintf(cmd, "patch -R %s < %s 2>&1 > %s",
          text_name, diff->name, errs_name);
  stash_log(STASH_DEBUG, "running: %s", cmd);

  rc = system(cmd);
  stash_log(STASH_DEBUG, "exit code: %i", rc);

  bool b = patch_errs(rc, errs_name);
  CHECK(b, "error applying patch");

  return true;
}

/** Reports the output from patch if verbose or on error */
static bool
patch_errs(int rc, const char* errs_name)
{
  if (rc == 0 && stash_verbosity <= STASH_INFO) return true;

  if (rc != 0) stash_log(STASH_WARN, "patch returned an error:");

  struct stat s;
  stat(errs_name, &s);
  if (s.st_size > 0)
  {
    char* errs = slurp(errs_name);
    CHECK(errs != NULL, "error reading errs file");
    puts(errs);
    free(errs);
  }

  if (rc != 0) return false;
  return true;
}

void
stash_abort(const char* format, ...)
{
  char buffer[path_max*3];
  int count = 0;
  char* p = &buffer[0];
  va_list ap;
  va_start(ap, format);
  count += sprintf(p, "stash: abort: ");
  count += vsnprintf(buffer+count, (size_t)(path_max*3-count),
                     format, ap);
  va_end(ap);
  printf("%s\n", buffer);
  fflush(NULL);
  exit(EXIT_FAILURE);
}
