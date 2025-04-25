#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include "md5_SSE.h"
#include <iomanip>
#include <emmintrin.h>
using namespace std;
using namespace chrono;

// 编译指令如下：
// g++ correctness.cpp train.cpp guessing.cpp md5.cpp -o test.exe


// 通过这个函数，你可以验证你实现的SIMD哈希函数的正确性
int main()
{
    // bit32 state[4];
    // MD5Hash("bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva", state);
    // for (int i1 = 0; i1 < 4; i1 += 1)
    // {
    //     cout << std::setw(8) << std::setfill('0') << hex << state[i1];
    // }
    // cout << endl;

    __m128i state[4];
    string pw[4];
    for(int i =0;i<4;i++){
        pw[i] = "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva";
    }
    MD5Hash_SIMD(pw,state);

    uint32_t outA[4], outB[4], outC[4], outD[4];

    _mm_storeu_si128((__m128i*)outA, state[0]);
    _mm_storeu_si128((__m128i*)outB, state[1]);
    _mm_storeu_si128((__m128i*)outC, state[2]);
    _mm_storeu_si128((__m128i*)outD, state[3]);

    for (int i1 = 0; i1 < 4; i1++) {
        uint32_t a = outA[i1];
        uint32_t b = outB[i1];
        uint32_t c = outC[i1];
        uint32_t d = outD[i1];
        cout << setw(8) << setfill('0') << hex << a
            << setw(8) << setfill('0') << hex << b
            << setw(8) << setfill('0') << hex << c
            << setw(8) << setfill('0') << hex << d << endl;
    }

}