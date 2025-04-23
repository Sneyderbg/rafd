#pragma once

#include <malloc.h>
#include <stddef.h>
#include <stdio.h>

#define DA_append(da, item)                                                    \
  {                                                                            \
    if (da.len >= da.capacity) {                                               \
      if (da.capacity == 0)                                                    \
        da.capacity = 10;                                                      \
      else                                                                     \
        da.capacity *= 2;                                                      \
      da.items = realloc(da.items, da.capacity * sizeof(*da.items));           \
    }                                                                          \
    da.items[da.len++] = item;                                                 \
  }

#define DA_new(name, type)                                                     \
  struct {                                                                     \
    type *items;                                                               \
    size_t len;                                                                \
    size_t capacity;                                                           \
  } name

#define DA_free(da)                                                            \
  if (da.items != NULL) {                                                      \
    free(da.items);                                                            \
  }

#define TODO(msg)                                                              \
  fprintf(stderr, "TODO: %s\n", msg);                                          \
  exit(1)
