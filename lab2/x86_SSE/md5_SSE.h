#include <iostream>
#include <string>
#include <cstring>
#include <emmintrin.h>

using namespace std;

// 定义了Byte，便于使用
typedef unsigned char Byte;
// 定义了32比特
typedef unsigned int bit32;

// MD5的一系列参数。参数是固定的，其实你不需要看懂这些
#define s11 7
#define s12 12
#define s13 17
#define s14 22
#define s21 5
#define s22 9
#define s23 14
#define s24 20
#define s31 4
#define s32 11
#define s33 16
#define s34 23
#define s41 6
#define s42 10
#define s43 15
#define s44 21

/**
 * @Basic MD5 functions.
 *
 * @param there bit32.
 *
 * @return one bit32.
 */



/**
 * @Rotate Left.
 *
 * @param {num} the raw number.
 *
 * @param {n} rotate left n.
 *
 * @return the number after rotated left.
 */


// 基础位操作宏
#define SSE_AND(a, b)       _mm_and_si128(a, b)
#define SSE_OR(a, b)        _mm_or_si128(a, b)
#define SSE_XOR(a, b)       _mm_xor_si128(a, b)
#define SSE_ANDNOT(a, b)    _mm_andnot_si128(a, b)
#define SSE_NOT(a)          SSE_XOR(a, _mm_set1_epi32(0xFFFFFFFF))

// 位选择宏 (vbslq_u32的SSE等效)
#define SSE_BSL(mask, a, b) SSE_OR(SSE_AND(mask, a), SSE_ANDNOT(mask, b))

// 位移宏
#define SSE_SHL(a, n)       _mm_slli_epi32(a, n)
#define SSE_SHR(a, n)       _mm_srli_epi32(a, n)

// 核心逻辑宏
#define F_SIMD_SSE(x, y, z)    SSE_BSL(x, y, z)
#define G_SIMD_SSE(x, y, z)    SSE_BSL(z, x, y)
#define H_SIMD_SSE(x, y, z)    SSE_XOR(SSE_XOR(x, y), z)
#define I_SIMD_SSE(x, y, z)    SSE_XOR(y, SSE_OR(x, SSE_NOT(z)))

// 循环左移宏（修正版）
#define ROTATELEFT_SSE(num, n) \
    SSE_OR(SSE_SHL(num, n), SSE_SHR(num, 32 - n))

// 轮函数宏
#define FF_SIMD_SSE(a, b, c, d, x, s, ac) { \
    __m128i vac = _mm_set1_epi32(ac);     \
    a = _mm_add_epi32(a, F_SIMD_SSE(b, c, d)); \
    a = _mm_add_epi32(a, x);                 \
    a = _mm_add_epi32(a, vac);               \
    a = ROTATELEFT_SSE(a, s);               \
    a = _mm_add_epi32(a, b);                 \
}

#define GG_SIMD_SSE(a, b, c, d, x, s, ac) { \
    __m128i vac = _mm_set1_epi32(ac);     \
    a = _mm_add_epi32(a, G_SIMD_SSE(b, c, d)); \
    a = _mm_add_epi32(a, x);                 \
    a = _mm_add_epi32(a, vac);               \
    a = ROTATELEFT_SSE(a, s);               \
    a = _mm_add_epi32(a, b);                 \
}

#define HH_SIMD_SSE(a, b, c, d, x, s, ac) { \
    __m128i vac = _mm_set1_epi32(ac);     \
    a = _mm_add_epi32(a, H_SIMD_SSE(b, c, d)); \
    a = _mm_add_epi32(a, x);                 \
    a = _mm_add_epi32(a, vac);               \
    a = ROTATELEFT_SSE(a, s);               \
    a = _mm_add_epi32(a, b);                 \
}

#define II_SIMD_SSE(a, b, c, d, x, s, ac) { \
    __m128i vac = _mm_set1_epi32(ac);     \
    a = _mm_add_epi32(a, I_SIMD_SSE(b, c, d)); \
    a = _mm_add_epi32(a, x);                 \
    a = _mm_add_epi32(a, vac);               \
    a = ROTATELEFT_SSE(a, s);               \
    a = _mm_add_epi32(a, b);                 \
}

//实现一个将字节序反转的宏，在MD5Hash_SIMD()用到
#define SHUFFLE_128_FIXED(x) ({ \
    __m128i t1 = _mm_and_si128(x, _mm_set1_epi32(0x000000FF)); \
    __m128i t2 = _mm_and_si128(x, _mm_set1_epi32(0x0000FF00)); \
    __m128i t3 = _mm_and_si128(x, _mm_set1_epi32(0x00FF0000)); \
    __m128i t4 = _mm_and_si128(x, _mm_set1_epi32(0xFF000000)); \
    t1 = _mm_slli_epi32(t1, 24); \
    t2 = _mm_slli_epi32(t2, 8); \
    t3 = _mm_srli_epi32(t3, 8); \
    t4 = _mm_srli_epi32(t4, 24); \
    _mm_or_si128(_mm_or_si128(t1, t2), _mm_or_si128(t3, t4)); \
})

void MD5Hash_SIMD(string* input, __m128i* state);
