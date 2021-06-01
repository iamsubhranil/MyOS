#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t    siz;
typedef uintptr_t uptr;
typedef intptr_t  iptr;

struct Limits {
	static const siz SizMax = SIZE_MAX;
	static const u32 U32Max = UINT32_MAX;
};
