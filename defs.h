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

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define SCR_W 320
#define SCR_H 200
#define SCALE 1
#define PAGE_SIZE (SCR_W*SCR_H)

typedef struct {
  u8 *bytes;
  size_t n;
  char *name;
} byte_array;

#endif
