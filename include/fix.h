#pragma once
#include <Arduino.h>

/* FIX15 (Q16.15) Macros */
typedef int32_t fix15;
#define multiply_fix15(a, b) ((fix15)((((int64_t)(a)) * ((int64_t)(b))) >> 15))
#define divide_fix15(a, b)   ((fix15)(((int64_t)(a) << 15) / (b)))
#define float2fix15(a)       ((fix15)((a)*32768.0)) // 2^15
#define fix2float15(a)       ((float)(a) / 32768.0) // 2^15
#define int2fix15(a)         ((fix15)((a) << 15))
#define fix2int15(a)         ((int)((a) >> 15))