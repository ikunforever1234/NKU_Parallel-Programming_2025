#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5_simd.h"
#include "md5.h"
#include <arm_neon.h>
#include <iomanip>
using namespace std;
using namespace chrono;

// 编译指令如下：
// g++ correctness.cpp train.cpp guessing.cpp md5.cpp -o main


// 通过这个函数，你可以验证你实现的SIMD哈希函数的正确性
int main()
{
    uint32x4_t state[4];
    //bit32 state[4];
    string pw[4];
    for(int i =0;i<4;i++){
        pw[i] = "bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva";
    }
    MD5Hash_SIMD_hong(pw,state);
    // MD5Hash_SIMD(pw,state);
    //MD5Hash("bvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdvabvaisdbjasdkafkasdfnavkjnakdjfejfanjsdnfkajdfkajdfjkwanfdjaknsvjkanbjbjadfajwefajksdfakdnsvjadfasjdva", state);
    //MD5Hash("b", state);
    // for (int i1 = 0; i1 < 4; i1 += 1)
    // {
    //     cout << std::setw(8) << std::setfill('0') << hex << state[i1];
    // }
    
    uint32_t out1[4],out2[4],out3[4],out4[4];
    vst1q_u32(out1, state[0]);
    vst1q_u32(out2, state[1]);
    vst1q_u32(out3, state[2]);
    vst1q_u32(out4, state[3]);
    for(int i=0;i<4;i += 1){
        cout << setw(8)<< setfill('0')<< hex << out1[i]
             << setw(8)<< setfill('0')<< hex << out2[i]
             << setw(8)<< setfill('0')<< hex << out3[i]
             << setw(8)<< setfill('0')<< hex << out4[i] << endl;
    }
}