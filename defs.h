#ifndef __DEFS_H__
#define __DEFS_H__

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define SCR_W 320
#define SCR_H 200
#define SCALE 1
#define PAGE_SIZE (SCR_W*SCR_H)

typedef struct {
  u8 *bytes;
  size_t n;
  char *name;
} byte_array;

int read_array(byte_array *o, const char *fname) {

  FILE *f = fopen(fname, "rb");

  if (NULL == f) {
    fprintf(stderr, "couldn't open %s\n", fname);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  o->n = ftell(f);
  o->name = strdup(fname);
  rewind(f);

  printf("read %s (%zu B)\n", fname, o->n);

  o->bytes = malloc(o->n * sizeof(u8));
  fread(o->bytes, sizeof(u8), o->n, f);

  fclose(f);

  return 0;
}

#endif
