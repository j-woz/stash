
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

/** The amount of data that can be added in one function call */
static size_t buffer_chunk_size = 1024;

bool
buffer_init(buffer* B, size_t capacity)
{
  B->data = malloc(capacity);
  if (B->data == NULL)
    return false;
  B->length = 0;
  B->capacity = capacity;
  return true;
}

void
buffer_reset(buffer* B)
{
  B->length = 0;
}

void
buffer_chunk_size_set(size_t c)
{
  buffer_chunk_size = c;
}

size_t
buffer_chunk_size_get(void)
{
  return buffer_chunk_size;
}

char*
buffer_dup(buffer* B)
{
  return strdup(B->data);
}

bool
buffer_append(buffer* B, const char* text)
{
  int count = strlen(text);
  return buffer_append_data(B, text, count);
}

bool
buffer_appendv(buffer* B, const char* format, ...)
{
  char chunk[buffer_chunk_size];
  va_list ap;
  va_start(ap, format);
  int count = vsnprintf(chunk, buffer_chunk_size, format, ap);
  va_end(ap);

  if (count >= buffer_chunk_size)
    return false;

  return buffer_append_data(B, chunk, count);
}

bool
buffer_append_data(buffer* B, const char* data, int count)
{
  if (B->length + count > B->capacity)
  {
    size_t c2 = B->capacity * 2;
    // printf("realloc: %zi -> %zi\n", b->capacity, c2);
    B->capacity = c2;
    char* new = realloc(B->data, c2);
    if (new == NULL)
      return false;
    B->data = new;
  }
  char* tail = B->data + B->length;
  strcpy(tail, data);
  B->length += count;
  return true;
}

void
buffer_finalize(buffer* B)
{
  free(B->data);
}

void
buffer_free(buffer* B)
{
  buffer_finalize(B);
  free(B);
}
