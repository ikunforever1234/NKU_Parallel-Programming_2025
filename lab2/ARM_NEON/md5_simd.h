#include <iostream>
#include <string>
#include <cstring>
#include<arm_neon.h>
using namespace std;

// 定义了Byte，便于使用
typedef unsigned char Byte;
// 定义了32比特
typedef unsigned int bit32;

/**
 * @Basic MD5 functions.
 *
 * @param there bit32.
 *
 * @return one bit32.
 */

uint32x4_t F_SIMD(uint32x4_t x, uint32x4_t y, uint32x4_t z);

uint32x4_t G_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z);

uint32x4_t H_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z);

uint32x4_t I_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z);

uint32x4_t ROTATELEFT_SIMD(uint32x4_t num, int n);

void FF_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac);

void GG_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac);

void HH_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac);

void II_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac);

void MD5Hash_SIMD(string* input, uint32x4_t *state);

#define F_SIMD_hong(x, y, z) vbslq_u32(x, y, z)


#define G_SIMD_hong(x, y, z) vbslq_u32(z, x, y)


#define H_SIMD_hong(x, y, z) veorq_u32(veorq_u32(x, y), z)


#define I_SIMD_hong(x, y, z) veorq_u32(y, vorrq_u32(x, vmvnq_u32(z)))


#define ROTATELEFT_SIMD_hong(num, n) \
    vorrq_u32( \
        vshlq_u32(num, vdupq_n_s32(n)), \
        vshlq_u32(num, vdupq_n_s32((n - 32))) \
    )


#define FF_SIMD_hong(a, b, c, d, x, s, ac) { \
    uint32x4_t vac = vdupq_n_u32(ac); \
    a = vaddq_u32(a, F_SIMD_hong(b, c, d)); \
    a = vaddq_u32(a, x); \
    a = vaddq_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong(a, s); \
    a = vaddq_u32(a, b); \
}


#define GG_SIMD_hong(a, b, c, d, x, s, ac) { \
    uint32x4_t vac = vdupq_n_u32(ac); \
    a = vaddq_u32(a, G_SIMD_hong(b, c, d)); \
    a = vaddq_u32(a, x); \
    a = vaddq_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong(a, s); \
    a = vaddq_u32(a, b); \
}


#define HH_SIMD_hong(a, b, c, d, x, s, ac) { \
    uint32x4_t vac = vdupq_n_u32(ac); \
    a = vaddq_u32(a, H_SIMD_hong(b, c, d)); \
    a = vaddq_u32(a, x); \
    a = vaddq_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong(a, s); \
    a = vaddq_u32(a, b); \
}


#define II_SIMD_hong(a, b, c, d, x, s, ac) { \
    uint32x4_t vac = vdupq_n_u32(ac); \
    a = vaddq_u32(a, I_SIMD_hong(b, c, d)); \
    a = vaddq_u32(a, x); \
    a = vaddq_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong(a, s); \
    a = vaddq_u32(a, b); \
}

#define F_SIMD_hong_2way(x, y, z) vbsl_u32(x, y, z)


#define G_SIMD_hong_2way(x, y, z) vbsl_u32(z, x, y)


#define H_SIMD_hong_2way(x, y, z) veor_u32(veor_u32(x, y), z)


#define I_SIMD_hong_2way(x, y, z) veor_u32(y, vorr_u32(x, vmvn_u32(z)))


#define ROTATELEFT_SIMD_hong_2way(num, n) \
    vorr_u32( \
        vshl_u32(num, vdup_n_s32(n)), \
        vshl_u32(num, vdup_n_s32((n - 32))) \
    )

#define FF_SIMD_hong_2way(a, b, c, d, x, s, ac) { \
    uint32x2_t vac = vdup_n_u32(ac); \
    a = vadd_u32(a, F_SIMD_hong_2way(b, c, d)); \
    a = vadd_u32(a, x); \
    a = vadd_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong_2way(a, s); \
    a = vadd_u32(a, b); \
}


#define GG_SIMD_hong_2way(a, b, c, d, x, s, ac) { \
    uint32x2_t vac = vdup_n_u32(ac); \
    a = vadd_u32(a, G_SIMD_hong_2way(b, c, d)); \
    a = vadd_u32(a, x); \
    a = vadd_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong_2way(a, s); \
    a = vadd_u32(a, b); \
}


#define HH_SIMD_hong_2way(a, b, c, d, x, s, ac) { \
    uint32x2_t vac = vdup_n_u32(ac); \
    a = vadd_u32(a, H_SIMD_hong_2way(b, c, d)); \
    a = vadd_u32(a, x); \
    a = vadd_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong_2way(a, s); \
    a = vadd_u32(a, b); \
}


#define II_SIMD_hong_2way(a, b, c, d, x, s, ac) { \
    uint32x2_t vac = vdup_n_u32(ac); \
    a = vadd_u32(a, I_SIMD_hong_2way(b, c, d)); \
    a = vadd_u32(a, x); \
    a = vadd_u32(a, vac); \
    a = ROTATELEFT_SIMD_hong_2way(a, s); \
    a = vadd_u32(a, b); \
}

void MD5Hash_SIMD_hong(string* input, uint32x4_t *state);
void MD5Hash_SIMD_hong_2way(string* input, uint32x2_t *state);