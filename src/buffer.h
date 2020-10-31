
#pragma once

#include <stdbool.h>
#include <string.h>

typedef struct
{
  /** Start of user data */
  char* data;
  size_t length;
  size_t capacity;
} buffer;

bool buffer_init(buffer* B, size_t capacity);

void buffer_reset(buffer* B);

void buffer_chunk_size_set(size_t c);

size_t buffer_chunk_size_get(void);

char* buffer_dup(buffer* B);

bool buffer_append(buffer* B, const char* text);

bool buffer_appendv(buffer* B, const char* format, ...);

bool buffer_append_data(buffer* B, const char* data, int count);

void buffer_finalize(buffer* B);

void buffer_free(buffer* B);
