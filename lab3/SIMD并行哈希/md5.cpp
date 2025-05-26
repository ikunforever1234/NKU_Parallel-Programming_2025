#include "md5.h"
#include "md5_simd.h"
#include <iomanip>
#include <assert.h>
#include <arm_neon.h>
#include <chrono>

using namespace std;
using namespace chrono;

uint32x4_t F_SIMD(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
    // x为1则选择y，x为0则选择z
    return vbslq_u32(x, y, z);
}

uint32x4_t G_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z){
    //原理同上，无非是反过来
    return vbslq_u32(z, x, y);
}

uint32x4_t H_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z){
    //异或
    uint32x4_t temp = veorq_u32(x,y);
    return veorq_u32(temp , z);
}

uint32x4_t I_SIMD(uint32x4_t x,uint32x4_t y,uint32x4_t z){
    uint32x4_t notz =  vmvnq_u32 (z);//非
    uint32x4_t temp = vorrq_u32(x , notz);//或
    return veorq_u32(y , temp);
}

uint32x4_t ROTATELEFT_SIMD(uint32x4_t num, int n){
    int32x4_t left = vdupq_n_s32(n);//左移位数向量
    int32x4_t right = vdupq_n_s32(n - 32);//右移向量，右移（32-n）相当于左移（n-32）

    uint32x4_t move_left = vshlq_u32(num , left);//左移操作
    uint32x4_t move_right = vshlq_u32(num , right);

    return vorrq_u32(move_left , move_right);
}

void FF_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac){
    uint32x4_t vac = vdupq_n_u32(ac);

    a = vaddq_u32(a , F_SIMD(b,c,d));
    a = vaddq_u32(a , x);
    a = vaddq_u32(a , vac);

    a = ROTATELEFT_SIMD(a , s);

    a = vaddq_u32(a , b);
}

void GG_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac){
    uint32x4_t vac = vdupq_n_u32(ac);

    a = vaddq_u32(a , G_SIMD(b,c,d));
    a = vaddq_u32(a , x);
    a = vaddq_u32(a , vac);

    a = ROTATELEFT_SIMD(a , s);

    a = vaddq_u32(a , b);
}

void HH_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac){
    uint32x4_t vac = vdupq_n_u32(ac);

    a = vaddq_u32(a , H_SIMD(b,c,d));
    a = vaddq_u32(a , x);
    a = vaddq_u32(a , vac);

    a = ROTATELEFT_SIMD(a , s);

    a = vaddq_u32(a , b);
}

void II_SIMD(uint32x4_t& a,uint32x4_t b,uint32x4_t c,uint32x4_t d,uint32x4_t x,int s,uint32_t ac){
    uint32x4_t vac = vdupq_n_u32(ac);

    a = vaddq_u32(a , I_SIMD(b,c,d));
    a = vaddq_u32(a , x);
    a = vaddq_u32(a , vac);

    a = ROTATELEFT_SIMD(a , s);

    a = vaddq_u32(a , b);
}

/**
 * StringProcess: 将单个输入字符串转换成MD5计算所需的消息数组
 * @param input 输入
 * @param[out] n_byte 用于给调用者传递额外的返回值，即最终Byte数组的长度
 * @return Byte消息数组
 */
Byte *StringProcess(string input, int *n_byte)
{
	// 将输入的字符串转换为Byte为单位的数组
	Byte *blocks = (Byte *)input.c_str();
	int length = input.length();

	// 计算原始消息长度（以比特为单位）
	int bitLength = length * 8;

	// paddingBits: 原始消息需要的padding长度（以bit为单位）
	// 对于给定的消息，将其补齐至length%512==448为止
	// 需要注意的是，即便给定的消息满足length%512==448，也需要再pad 512bits
	int paddingBits = bitLength % 512;
	if (paddingBits > 448)
	{
		paddingBits = 512 - (paddingBits - 448);
	}
	else if (paddingBits < 448)
	{
		paddingBits = 448 - paddingBits;
	}
	else if (paddingBits == 448)
	{
		paddingBits = 512;
	}

	// 原始消息需要的padding长度（以Byte为单位）
	int paddingBytes = paddingBits / 8;
	// 创建最终的字节数组
	// length + paddingBytes + 8:
	// 1. length为原始消息的长度（bits）
	// 2. paddingBytes为原始消息需要的padding长度（Bytes）
	// 3. 在pad到length%512==448之后，需要额外附加64bits的原始消息长度，即8个bytes
	int paddedLength = length + paddingBytes + 8;
	Byte *paddedMessage = new Byte[paddedLength];

	// 复制原始消息
	memcpy(paddedMessage, blocks, length);

	// 添加填充字节。填充时，第一位为1，后面的所有位均为0。
	// 所以第一个byte是0x80
	paddedMessage[length] = 0x80;							 // 添加一个0x80字节
	memset(paddedMessage + length + 1, 0, paddingBytes - 1); // 填充0字节

	// 添加消息长度（64比特，小端格式）
	for (int i = 0; i < 8; ++i)
	{
		// 特别注意此处应当将bitLength转换为uint64_t
		// 这里的length是原始消息的长度
		paddedMessage[length + paddingBytes + i] = ((uint64_t)length * 8 >> (i * 8)) & 0xFF;
	}

	// // 验证长度是否满足要求。此时长度应当是512bit的倍数
	// int residual = 8 * paddedLength;
	// // assert(residual == 0);

	// cout<<residual<<endl;
	// for(int i= 0;i<residual;i++){
	// 	cout<<paddedMessage[i];
	// }
	// cout<<endl;

	// 在填充+添加长度之后，消息被分为n_blocks个512bit的部分
	*n_byte = paddedLength;
	return paddedMessage;
}



Byte *StringProcess_SIMD(string input, int *n_byte)
{
	// 将输入的字符串转换为Byte为单位的数组
	Byte *blocks = (Byte *)input.c_str();
	int length = input.length();
	// 计算原始消息长度（以比特为单位）
	int bitLength = length * 8;

	// paddingBits: 原始消息需要的padding长度（以bit为单位）
	// 对于给定的消息，将其补齐至length%512==448为止
	// 需要注意的是，即便给定的消息满足length%512==448，也需要再pad 512bits
	int paddingBits = bitLength % 512;
	if (paddingBits > 448)
	{
		paddingBits = 512 - (paddingBits - 448);
	}
	else if (paddingBits < 448)
	{
		paddingBits = 448 - paddingBits;
	}
	else if (paddingBits == 448)
	{
		paddingBits = 512;
	}

	// 原始消息需要的padding长度（以Byte为单位）
	int paddingBytes = paddingBits / 8;
	// 创建最终的字节数组
	// length + paddingBytes + 8:
	// 1. length为原始消息的长度（bits）
	// 2. paddingBytes为原始消息需要的padding长度（Bytes）
	// 3. 在pad到length%512==448之后，需要额外附加64bits的原始消息长度，即8个bytes
	int paddedLength = length + paddingBytes + 8;
	Byte *paddedMessage = new Byte[paddedLength];

	//先并行处理满16字节的，然后串行处理未满16字节的
	int i = 0;
	for(;i+16<=length;i+=16){
		//将16字节的数据批量加载到NEON寄存器
		uint8x16_t vblock = vld1q_u8(blocks + i);//vld1q_u8()从内存加载数据
		//批量存储到目标地址
		vst1q_u8(paddedMessage + i, vblock);//vst1q_u8() 存回内存
	}
	memcpy(paddedMessage + i, blocks+i, length-i);

	
	paddedMessage[length] = 0x80;

	int pad_0_start = length + 1;
	int pad_0_end = length + paddingBytes;

	//计算对齐部分填充的开始和结束，如填充起始为10，则从16开始填充
	int can_simd_pad_start = (pad_0_start + 16 - 1) & ~(16-1);
	int can_simd_pad_size = pad_0_end - can_simd_pad_start;

	//填充前面的未对齐部分
	memset(paddedMessage + pad_0_start, 0, can_simd_pad_start - pad_0_start);

	//并行填充可simd部分
	uint8x16_t vec_0 = vmovq_n_u8(0);
	for(int j = can_simd_pad_start; j+16<=pad_0_end;j+=16){
		vst1q_u8(paddedMessage+j , vec_0);
	}

	//填充后面的未对齐部分
	memset(paddedMessage + (pad_0_end-(can_simd_pad_size%16)), 0, can_simd_pad_size%16);

	int len_start = length + paddingBytes;
	uint64_t xiaodun_length = htole64(bitLength);//转化为小端序

	uint8x8_t vec_len = vcreate_u8(xiaodun_length);//长度转化为向量
	vst1_u8(paddedMessage + len_start, vec_len);

	// int residual = 8 * paddedLength ;
	// cout<<residual<<endl;
	// for(int i= 0;i<residual;i++){
	// 	cout<<paddedMessage[i];
	// }
	// cout<<endl;

	*n_byte = paddedLength;
	return paddedMessage;
}


/**
 * MD5Hash: 将单个输入字符串转换成MD5
 * @param input 输入
 * @param[out] state 用于给调用者传递额外的返回值，即最终的缓冲区，也就是MD5的结果
 * @return Byte消息数组
 */
void MD5Hash(string input, bit32 *state)
{

	Byte *paddedMessage;
	int *messageLength = new int[1];
	for (int i = 0; i < 1; i += 1)
	{
		paddedMessage = StringProcess(input, &messageLength[i]);
		// cout<<messageLength[i]<<endl;
		assert(messageLength[i] == messageLength[0]);
	}
	int n_blocks = messageLength[0] / 64;

	// bit32* state= new bit32[4];
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	// 逐block地更新state
	for (int i = 0; i < n_blocks; i += 1)
	{
		bit32 x[16];

		// 下面的处理，在理解上较为复杂
		for (int i1 = 0; i1 < 16; ++i1)
		{
			x[i1] = (paddedMessage[4 * i1 + i * 64]) |
					(paddedMessage[4 * i1 + 1 + i * 64] << 8) |
					(paddedMessage[4 * i1 + 2 + i * 64] << 16) |
					(paddedMessage[4 * i1 + 3 + i * 64] << 24);
		}

		bit32 a = state[0], b = state[1], c = state[2], d = state[3];

		auto start = system_clock::now();
		/* Round 1 */
		FF(a, b, c, d, x[0], s11, 0xd76aa478);
		FF(d, a, b, c, x[1], s12, 0xe8c7b756);
		FF(c, d, a, b, x[2], s13, 0x242070db);
		FF(b, c, d, a, x[3], s14, 0xc1bdceee);
		FF(a, b, c, d, x[4], s11, 0xf57c0faf);
		FF(d, a, b, c, x[5], s12, 0x4787c62a);
		FF(c, d, a, b, x[6], s13, 0xa8304613);
		FF(b, c, d, a, x[7], s14, 0xfd469501);
		FF(a, b, c, d, x[8], s11, 0x698098d8);
		FF(d, a, b, c, x[9], s12, 0x8b44f7af);
		FF(c, d, a, b, x[10], s13, 0xffff5bb1);
		FF(b, c, d, a, x[11], s14, 0x895cd7be);
		FF(a, b, c, d, x[12], s11, 0x6b901122);
		FF(d, a, b, c, x[13], s12, 0xfd987193);
		FF(c, d, a, b, x[14], s13, 0xa679438e);
		FF(b, c, d, a, x[15], s14, 0x49b40821);

		/* Round 2 */
		GG(a, b, c, d, x[1], s21, 0xf61e2562);
		GG(d, a, b, c, x[6], s22, 0xc040b340);
		GG(c, d, a, b, x[11], s23, 0x265e5a51);
		GG(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		GG(a, b, c, d, x[5], s21, 0xd62f105d);
		GG(d, a, b, c, x[10], s22, 0x2441453);
		GG(c, d, a, b, x[15], s23, 0xd8a1e681);
		GG(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		GG(a, b, c, d, x[9], s21, 0x21e1cde6);
		GG(d, a, b, c, x[14], s22, 0xc33707d6);
		GG(c, d, a, b, x[3], s23, 0xf4d50d87);
		GG(b, c, d, a, x[8], s24, 0x455a14ed);
		GG(a, b, c, d, x[13], s21, 0xa9e3e905);
		GG(d, a, b, c, x[2], s22, 0xfcefa3f8);
		GG(c, d, a, b, x[7], s23, 0x676f02d9);
		GG(b, c, d, a, x[12], s24, 0x8d2a4c8a);

		/* Round 3 */
		HH(a, b, c, d, x[5], s31, 0xfffa3942);
		HH(d, a, b, c, x[8], s32, 0x8771f681);
		HH(c, d, a, b, x[11], s33, 0x6d9d6122);
		HH(b, c, d, a, x[14], s34, 0xfde5380c);
		HH(a, b, c, d, x[1], s31, 0xa4beea44);
		HH(d, a, b, c, x[4], s32, 0x4bdecfa9);
		HH(c, d, a, b, x[7], s33, 0xf6bb4b60);
		HH(b, c, d, a, x[10], s34, 0xbebfbc70);
		HH(a, b, c, d, x[13], s31, 0x289b7ec6);
		HH(d, a, b, c, x[0], s32, 0xeaa127fa);
		HH(c, d, a, b, x[3], s33, 0xd4ef3085);
		HH(b, c, d, a, x[6], s34, 0x4881d05);
		HH(a, b, c, d, x[9], s31, 0xd9d4d039);
		HH(d, a, b, c, x[12], s32, 0xe6db99e5);
		HH(c, d, a, b, x[15], s33, 0x1fa27cf8);
		HH(b, c, d, a, x[2], s34, 0xc4ac5665);

		/* Round 4 */
		II(a, b, c, d, x[0], s41, 0xf4292244);
		II(d, a, b, c, x[7], s42, 0x432aff97);
		II(c, d, a, b, x[14], s43, 0xab9423a7);
		II(b, c, d, a, x[5], s44, 0xfc93a039);
		II(a, b, c, d, x[12], s41, 0x655b59c3);
		II(d, a, b, c, x[3], s42, 0x8f0ccc92);
		II(c, d, a, b, x[10], s43, 0xffeff47d);
		II(b, c, d, a, x[1], s44, 0x85845dd1);
		II(a, b, c, d, x[8], s41, 0x6fa87e4f);
		II(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		II(c, d, a, b, x[6], s43, 0xa3014314);
		II(b, c, d, a, x[13], s44, 0x4e0811a1);
		II(a, b, c, d, x[4], s41, 0xf7537e82);
		II(d, a, b, c, x[11], s42, 0xbd3af235);
		II(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		II(b, c, d, a, x[9], s44, 0xeb86d391);

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
	}

	// 下面的处理，在理解上较为复杂
	for (int i = 0; i < 4; i++)
	{
		uint32_t value = state[i];
		state[i] = ((value & 0xff) << 24) |		 // 将最低字节移到最高位
				   ((value & 0xff00) << 8) |	 // 将次低字节左移
				   ((value & 0xff0000) >> 8) |	 // 将次高字节右移
				   ((value & 0xff000000) >> 24); // 将最高字节移到最低位
	}

	// 输出最终的hash结果
	// for (int i1 = 0; i1 < 4; i1 += 1)
	// {
	// 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	// }
	// cout << endl;

	// 释放动态分配的内存
	// 实现SIMD并行算法的时候，也请记得及时回收内存！
	delete[] paddedMessage;
	delete[] messageLength;
}


void MD5Hash_SIMD(string* input, uint32x4_t *state)
 {
 
	Byte *paddedMessage[4];
	 int *messageLength = new int[4];
	 for (int i = 0; i < 4; i += 1)
	 {
		 paddedMessage[i] = StringProcess(input[i], &messageLength[i]);
		 // cout<<messageLength[i]<<endl;
		 //assert(messageLength[i] == messageLength[0]);
	 }
	 int n_blocks = messageLength[0] / 64;
 
	 // bit32* state= new bit32[4];
	 state[0] = vdupq_n_u32(0x67452301);
	 state[1] = vdupq_n_u32(0xefcdab89);
	 state[2] = vdupq_n_u32(0x98badcfe);
	 state[3] = vdupq_n_u32(0x10325476);
 
	 // 逐block地更新state
	 for (int i = 0; i < n_blocks; i += 1)
	 {
		uint32x4_t x[16];
 
		 // 下面的处理，在理解上较为复杂
		 for (int i1 = 0; i1 < 16; ++i1)
		 {
			uint32_t word[4];
			for(int j = 0;j<4;j++){	
				word[j] = (paddedMessage[j][4 * i1 + i * 64]) |
						  (paddedMessage[j][4 * i1 + 1 + i * 64] << 8) |
				  		  (paddedMessage[j][4 * i1 + 2 + i * 64] << 16) |
						  (paddedMessage[j][4 * i1 + 3 + i * 64] << 24);
			}
			x[i1] = vld1q_u32(word);
		 }
 
		 uint32x4_t a = state[0], b = state[1], c = state[2], d = state[3];
 
		 auto start = system_clock::now();
		 /* Round 1 */
		 FF_SIMD(a, b, c, d, x[0], s11, 0xd76aa478);
		 FF_SIMD(d, a, b, c, x[1], s12, 0xe8c7b756);
		 FF_SIMD(c, d, a, b, x[2], s13, 0x242070db);
		 FF_SIMD(b, c, d, a, x[3], s14, 0xc1bdceee);
		 FF_SIMD(a, b, c, d, x[4], s11, 0xf57c0faf);
		 FF_SIMD(d, a, b, c, x[5], s12, 0x4787c62a);
		 FF_SIMD(c, d, a, b, x[6], s13, 0xa8304613);
		 FF_SIMD(b, c, d, a, x[7], s14, 0xfd469501);
		 FF_SIMD(a, b, c, d, x[8], s11, 0x698098d8);
		 FF_SIMD(d, a, b, c, x[9], s12, 0x8b44f7af);
		 FF_SIMD(c, d, a, b, x[10], s13, 0xffff5bb1);
		 FF_SIMD(b, c, d, a, x[11], s14, 0x895cd7be);
		 FF_SIMD(a, b, c, d, x[12], s11, 0x6b901122);
		 FF_SIMD(d, a, b, c, x[13], s12, 0xfd987193);
		 FF_SIMD(c, d, a, b, x[14], s13, 0xa679438e);
		 FF_SIMD(b, c, d, a, x[15], s14, 0x49b40821);
 
		 /* Round 2 */
		 GG_SIMD(a, b, c, d, x[1], s21, 0xf61e2562);
		 GG_SIMD(d, a, b, c, x[6], s22, 0xc040b340);
		 GG_SIMD(c, d, a, b, x[11], s23, 0x265e5a51);
		 GG_SIMD(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		 GG_SIMD(a, b, c, d, x[5], s21, 0xd62f105d);
		 GG_SIMD(d, a, b, c, x[10], s22, 0x2441453);
		 GG_SIMD(c, d, a, b, x[15], s23, 0xd8a1e681);
		 GG_SIMD(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		 GG_SIMD(a, b, c, d, x[9], s21, 0x21e1cde6);
		 GG_SIMD(d, a, b, c, x[14], s22, 0xc33707d6);
		 GG_SIMD(c, d, a, b, x[3], s23, 0xf4d50d87);
		 GG_SIMD(b, c, d, a, x[8], s24, 0x455a14ed);
		 GG_SIMD(a, b, c, d, x[13], s21, 0xa9e3e905);
		 GG_SIMD(d, a, b, c, x[2], s22, 0xfcefa3f8);
		 GG_SIMD(c, d, a, b, x[7], s23, 0x676f02d9);
		 GG_SIMD(b, c, d, a, x[12], s24, 0x8d2a4c8a);
 
		 /* Round 3 */
		 HH_SIMD(a, b, c, d, x[5], s31, 0xfffa3942);
		 HH_SIMD(d, a, b, c, x[8], s32, 0x8771f681);
		 HH_SIMD(c, d, a, b, x[11], s33, 0x6d9d6122);
		 HH_SIMD(b, c, d, a, x[14], s34, 0xfde5380c);
		 HH_SIMD(a, b, c, d, x[1], s31, 0xa4beea44);
		 HH_SIMD(d, a, b, c, x[4], s32, 0x4bdecfa9);
		 HH_SIMD(c, d, a, b, x[7], s33, 0xf6bb4b60);
		 HH_SIMD(b, c, d, a, x[10], s34, 0xbebfbc70);
		 HH_SIMD(a, b, c, d, x[13], s31, 0x289b7ec6);
		 HH_SIMD(d, a, b, c, x[0], s32, 0xeaa127fa);
		 HH_SIMD(c, d, a, b, x[3], s33, 0xd4ef3085);
		 HH_SIMD(b, c, d, a, x[6], s34, 0x4881d05);
		 HH_SIMD(a, b, c, d, x[9], s31, 0xd9d4d039);
		 HH_SIMD(d, a, b, c, x[12], s32, 0xe6db99e5);
		 HH_SIMD(c, d, a, b, x[15], s33, 0x1fa27cf8);
		 HH_SIMD(b, c, d, a, x[2], s34, 0xc4ac5665);
 
		 /* Round 4 */
		 II_SIMD(a, b, c, d, x[0], s41, 0xf4292244);
		 II_SIMD(d, a, b, c, x[7], s42, 0x432aff97);
		 II_SIMD(c, d, a, b, x[14], s43, 0xab9423a7);
		 II_SIMD(b, c, d, a, x[5], s44, 0xfc93a039);
		 II_SIMD(a, b, c, d, x[12], s41, 0x655b59c3);
		 II_SIMD(d, a, b, c, x[3], s42, 0x8f0ccc92);
		 II_SIMD(c, d, a, b, x[10], s43, 0xffeff47d);
		 II_SIMD(b, c, d, a, x[1], s44, 0x85845dd1);
		 II_SIMD(a, b, c, d, x[8], s41, 0x6fa87e4f);
		 II_SIMD(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		 II_SIMD(c, d, a, b, x[6], s43, 0xa3014314);
		 II_SIMD(b, c, d, a, x[13], s44, 0x4e0811a1);
		 II_SIMD(a, b, c, d, x[4], s41, 0xf7537e82);
		 II_SIMD(d, a, b, c, x[11], s42, 0xbd3af235);
		 II_SIMD(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		 II_SIMD(b, c, d, a, x[9], s44, 0xeb86d391);
 
		 state[0] = vaddq_u32(state[0],a);
		 state[1] = vaddq_u32(state[1],b);
		 state[2] = vaddq_u32(state[2],c);
		 state[3] = vaddq_u32(state[3],d);
	 }
 
	 // 下面的处理，在理解上较为复杂
	 for (int i = 0; i < 4; i++)
	 {
		state[i] = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(state[i])));
	 }
 
	 // 输出最终的hash结果
	 // for (int i1 = 0; i1 < 4; i1 += 1)
	 // {
	 // 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	 // }
	 // cout << endl;
 
	 // 释放动态分配的内存
	 // 实现SIMD并行算法的时候，也请记得及时回收内存！
	 for(int i = 0;i < 4; i++){
		delete paddedMessage[i];
	 }
	 delete[] messageLength;
 }

 void MD5Hash_SIMD_hong(string* input, uint32x4_t *state)
 {
 
	 Byte *paddedMessage[4];
	 int *messageLength = new int[4];
	 for (int i = 0; i < 4; i += 1)
	 {
		 paddedMessage[i] = StringProcess(input[i], &messageLength[i]);
		 // cout<<messageLength[i]<<endl;
		 //assert(messageLength[i] == messageLength[0]);
	 }
	 int n_blocks = messageLength[0] / 64;
 
	 // bit32* state= new bit32[4];
	 state[0] = vdupq_n_u32(0x67452301);
	 state[1] = vdupq_n_u32(0xefcdab89);
	 state[2] = vdupq_n_u32(0x98badcfe);
	 state[3] = vdupq_n_u32(0x10325476);
 
	 // 逐block地更新state
	 for (int i = 0; i < n_blocks; i += 1)
	 {
		uint32x4_t x[16];
 
		 // 下面的处理，在理解上较为复杂
		 for (int i1 = 0; i1 < 16; ++i1)
		 {
			uint32_t word[4];
			for(int j = 0;j<4;j++){	
				word[j] = (paddedMessage[j][4 * i1 + i * 64]) |
						  (paddedMessage[j][4 * i1 + 1 + i * 64] << 8) |
				  		  (paddedMessage[j][4 * i1 + 2 + i * 64] << 16) |
						  (paddedMessage[j][4 * i1 + 3 + i * 64] << 24);
			}
			x[i1] = vld1q_u32(word);
		 }
 
		 uint32x4_t a = state[0], b = state[1], c = state[2], d = state[3];
 
		 auto start = system_clock::now();
		 /* Round 1 */
		 FF_SIMD_hong(a, b, c, d, x[0], s11, 0xd76aa478);
		 FF_SIMD_hong(d, a, b, c, x[1], s12, 0xe8c7b756);
		 FF_SIMD_hong(c, d, a, b, x[2], s13, 0x242070db);
		 FF_SIMD_hong(b, c, d, a, x[3], s14, 0xc1bdceee);
		 FF_SIMD_hong(a, b, c, d, x[4], s11, 0xf57c0faf);
		 FF_SIMD_hong(d, a, b, c, x[5], s12, 0x4787c62a);
		 FF_SIMD_hong(c, d, a, b, x[6], s13, 0xa8304613);
		 FF_SIMD_hong(b, c, d, a, x[7], s14, 0xfd469501);
		 FF_SIMD_hong(a, b, c, d, x[8], s11, 0x698098d8);
		 FF_SIMD_hong(d, a, b, c, x[9], s12, 0x8b44f7af);
		 FF_SIMD_hong(c, d, a, b, x[10], s13, 0xffff5bb1);
		 FF_SIMD_hong(b, c, d, a, x[11], s14, 0x895cd7be);
		 FF_SIMD_hong(a, b, c, d, x[12], s11, 0x6b901122);
		 FF_SIMD_hong(d, a, b, c, x[13], s12, 0xfd987193);
		 FF_SIMD_hong(c, d, a, b, x[14], s13, 0xa679438e);
		 FF_SIMD_hong(b, c, d, a, x[15], s14, 0x49b40821);
 
		 /* Round 2 */
		 GG_SIMD_hong(a, b, c, d, x[1], s21, 0xf61e2562);
		 GG_SIMD_hong(d, a, b, c, x[6], s22, 0xc040b340);
		 GG_SIMD_hong(c, d, a, b, x[11], s23, 0x265e5a51);
		 GG_SIMD_hong(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		 GG_SIMD_hong(a, b, c, d, x[5], s21, 0xd62f105d);
		 GG_SIMD_hong(d, a, b, c, x[10], s22, 0x2441453);
		 GG_SIMD_hong(c, d, a, b, x[15], s23, 0xd8a1e681);
		 GG_SIMD_hong(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		 GG_SIMD_hong(a, b, c, d, x[9], s21, 0x21e1cde6);
		 GG_SIMD_hong(d, a, b, c, x[14], s22, 0xc33707d6);
		 GG_SIMD_hong(c, d, a, b, x[3], s23, 0xf4d50d87);
		 GG_SIMD_hong(b, c, d, a, x[8], s24, 0x455a14ed);
		 GG_SIMD_hong(a, b, c, d, x[13], s21, 0xa9e3e905);
		 GG_SIMD_hong(d, a, b, c, x[2], s22, 0xfcefa3f8);
		 GG_SIMD_hong(c, d, a, b, x[7], s23, 0x676f02d9);
		 GG_SIMD_hong(b, c, d, a, x[12], s24, 0x8d2a4c8a);
 
		 /* Round 3 */
		 HH_SIMD_hong(a, b, c, d, x[5], s31, 0xfffa3942);
		 HH_SIMD_hong(d, a, b, c, x[8], s32, 0x8771f681);
		 HH_SIMD_hong(c, d, a, b, x[11], s33, 0x6d9d6122);
		 HH_SIMD_hong(b, c, d, a, x[14], s34, 0xfde5380c);
		 HH_SIMD_hong(a, b, c, d, x[1], s31, 0xa4beea44);
		 HH_SIMD_hong(d, a, b, c, x[4], s32, 0x4bdecfa9);
		 HH_SIMD_hong(c, d, a, b, x[7], s33, 0xf6bb4b60);
		 HH_SIMD_hong(b, c, d, a, x[10], s34, 0xbebfbc70);
		 HH_SIMD_hong(a, b, c, d, x[13], s31, 0x289b7ec6);
		 HH_SIMD_hong(d, a, b, c, x[0], s32, 0xeaa127fa);
		 HH_SIMD_hong(c, d, a, b, x[3], s33, 0xd4ef3085);
		 HH_SIMD_hong(b, c, d, a, x[6], s34, 0x4881d05);
		 HH_SIMD_hong(a, b, c, d, x[9], s31, 0xd9d4d039);
		 HH_SIMD_hong(d, a, b, c, x[12], s32, 0xe6db99e5);
		 HH_SIMD_hong(c, d, a, b, x[15], s33, 0x1fa27cf8);
		 HH_SIMD_hong(b, c, d, a, x[2], s34, 0xc4ac5665);
 
		 /* Round 4 */
		 II_SIMD_hong(a, b, c, d, x[0], s41, 0xf4292244);
		 II_SIMD_hong(d, a, b, c, x[7], s42, 0x432aff97);
		 II_SIMD_hong(c, d, a, b, x[14], s43, 0xab9423a7);
		 II_SIMD_hong(b, c, d, a, x[5], s44, 0xfc93a039);
		 II_SIMD_hong(a, b, c, d, x[12], s41, 0x655b59c3);
		 II_SIMD_hong(d, a, b, c, x[3], s42, 0x8f0ccc92);
		 II_SIMD_hong(c, d, a, b, x[10], s43, 0xffeff47d);
		 II_SIMD_hong(b, c, d, a, x[1], s44, 0x85845dd1);
		 II_SIMD_hong(a, b, c, d, x[8], s41, 0x6fa87e4f);
		 II_SIMD_hong(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		 II_SIMD_hong(c, d, a, b, x[6], s43, 0xa3014314);
		 II_SIMD_hong(b, c, d, a, x[13], s44, 0x4e0811a1);
		 II_SIMD_hong(a, b, c, d, x[4], s41, 0xf7537e82);
		 II_SIMD_hong(d, a, b, c, x[11], s42, 0xbd3af235);
		 II_SIMD_hong(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		 II_SIMD_hong(b, c, d, a, x[9], s44, 0xeb86d391);
 
		 state[0] = vaddq_u32(state[0],a);
		 state[1] = vaddq_u32(state[1],b);
		 state[2] = vaddq_u32(state[2],c);
		 state[3] = vaddq_u32(state[3],d);
	 }
 
	 for (int i = 0; i < 4; i++)
	 {
		state[i] = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(state[i])));
	 }
 
	 // 输出最终的hash结果
	 // for (int i1 = 0; i1 < 4; i1 += 1)
	 // {
	 // 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	 // }
	 // cout << endl;
 
	 // 释放动态分配的内存
	 // 实现SIMD并行算法的时候，也请记得及时回收内存！
	 for(int i = 0;i < 4; i++){
		delete paddedMessage[i];
	 }
	 delete[] messageLength;
 }

 void MD5Hash_SIMD_hong_2way(string* input, uint32x2_t *state)
 {
 
	 Byte *paddedMessage[2];
	 int *messageLength = new int[2];
	 for (int i = 0; i < 2; i += 1)
	 {
		 paddedMessage[i] = StringProcess(input[i], &messageLength[i]);
		 // cout<<messageLength[i]<<endl;
		 //assert(messageLength[i] == messageLength[0]);
	 }
	 int n_blocks = messageLength[0] / 64;
 
	 // bit32* state= new bit32[4];
	 state[0] = vdup_n_u32(0x67452301); 
     state[1] = vdup_n_u32(0xefcdab89);
     state[2] = vdup_n_u32(0x98badcfe);
     state[3] = vdup_n_u32(0x10325476);
 
	 // 逐block地更新state
	 for (int i = 0; i < n_blocks; i += 1)
	 {
		uint32x2_t x[16];
 
		 // 下面的处理，在理解上较为复杂
		 for (int i1 = 0; i1 < 16; ++i1)
		 {
			uint32_t word[2];
			for(int j = 0;j<2;j++){	
				word[j] =  (paddedMessage[j][4 * i1 + i * 64]) |
						  		(paddedMessage[j][4 * i1 + 1 + i * 64] << 8) |
				  		  		(paddedMessage[j][4 * i1 + 2 + i * 64] << 16) |
						  		(paddedMessage[j][4 * i1 + 3 + i * 64] << 24);
			}
			x[i1] = vld1_u32(word);
		 }
 
		 uint32x2_t a = state[0], b = state[1], c = state[2], d = state[3];
 
		 auto start = system_clock::now();
		 /* Round 1 */
		 FF_SIMD_hong_2way(a, b, c, d, x[0], s11, 0xd76aa478);
		 FF_SIMD_hong_2way(d, a, b, c, x[1], s12, 0xe8c7b756);
		 FF_SIMD_hong_2way(c, d, a, b, x[2], s13, 0x242070db);
		 FF_SIMD_hong_2way(b, c, d, a, x[3], s14, 0xc1bdceee);
		 FF_SIMD_hong_2way(a, b, c, d, x[4], s11, 0xf57c0faf);
		 FF_SIMD_hong_2way(d, a, b, c, x[5], s12, 0x4787c62a);
		 FF_SIMD_hong_2way(c, d, a, b, x[6], s13, 0xa8304613);
		 FF_SIMD_hong_2way(b, c, d, a, x[7], s14, 0xfd469501);
		 FF_SIMD_hong_2way(a, b, c, d, x[8], s11, 0x698098d8);
		 FF_SIMD_hong_2way(d, a, b, c, x[9], s12, 0x8b44f7af);
		 FF_SIMD_hong_2way(c, d, a, b, x[10], s13, 0xffff5bb1);
		 FF_SIMD_hong_2way(b, c, d, a, x[11], s14, 0x895cd7be);
		 FF_SIMD_hong_2way(a, b, c, d, x[12], s11, 0x6b901122);
		 FF_SIMD_hong_2way(d, a, b, c, x[13], s12, 0xfd987193);
		 FF_SIMD_hong_2way(c, d, a, b, x[14], s13, 0xa679438e);
		 FF_SIMD_hong_2way(b, c, d, a, x[15], s14, 0x49b40821);
 
		 /* Round 2 */
		 GG_SIMD_hong_2way(a, b, c, d, x[1], s21, 0xf61e2562);
		 GG_SIMD_hong_2way(d, a, b, c, x[6], s22, 0xc040b340);
		 GG_SIMD_hong_2way(c, d, a, b, x[11], s23, 0x265e5a51);
		 GG_SIMD_hong_2way(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		 GG_SIMD_hong_2way(a, b, c, d, x[5], s21, 0xd62f105d);
		 GG_SIMD_hong_2way(d, a, b, c, x[10], s22, 0x2441453);
		 GG_SIMD_hong_2way(c, d, a, b, x[15], s23, 0xd8a1e681);
		 GG_SIMD_hong_2way(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		 GG_SIMD_hong_2way(a, b, c, d, x[9], s21, 0x21e1cde6);
		 GG_SIMD_hong_2way(d, a, b, c, x[14], s22, 0xc33707d6);
		 GG_SIMD_hong_2way(c, d, a, b, x[3], s23, 0xf4d50d87);
		 GG_SIMD_hong_2way(b, c, d, a, x[8], s24, 0x455a14ed);
		 GG_SIMD_hong_2way(a, b, c, d, x[13], s21, 0xa9e3e905);
		 GG_SIMD_hong_2way(d, a, b, c, x[2], s22, 0xfcefa3f8);
		 GG_SIMD_hong_2way(c, d, a, b, x[7], s23, 0x676f02d9);
		 GG_SIMD_hong_2way(b, c, d, a, x[12], s24, 0x8d2a4c8a);
 
		 /* Round 3 */
		 HH_SIMD_hong_2way(a, b, c, d, x[5], s31, 0xfffa3942);
		 HH_SIMD_hong_2way(d, a, b, c, x[8], s32, 0x8771f681);
		 HH_SIMD_hong_2way(c, d, a, b, x[11], s33, 0x6d9d6122);
		 HH_SIMD_hong_2way(b, c, d, a, x[14], s34, 0xfde5380c);
		 HH_SIMD_hong_2way(a, b, c, d, x[1], s31, 0xa4beea44);
		 HH_SIMD_hong_2way(d, a, b, c, x[4], s32, 0x4bdecfa9);
		 HH_SIMD_hong_2way(c, d, a, b, x[7], s33, 0xf6bb4b60);
		 HH_SIMD_hong_2way(b, c, d, a, x[10], s34, 0xbebfbc70);
		 HH_SIMD_hong_2way(a, b, c, d, x[13], s31, 0x289b7ec6);
		 HH_SIMD_hong_2way(d, a, b, c, x[0], s32, 0xeaa127fa);
		 HH_SIMD_hong_2way(c, d, a, b, x[3], s33, 0xd4ef3085);
		 HH_SIMD_hong_2way(b, c, d, a, x[6], s34, 0x4881d05);
		 HH_SIMD_hong_2way(a, b, c, d, x[9], s31, 0xd9d4d039);
		 HH_SIMD_hong_2way(d, a, b, c, x[12], s32, 0xe6db99e5);
		 HH_SIMD_hong_2way(c, d, a, b, x[15], s33, 0x1fa27cf8);
		 HH_SIMD_hong_2way(b, c, d, a, x[2], s34, 0xc4ac5665);
 
		 /* Round 4 */
		 II_SIMD_hong_2way(a, b, c, d, x[0], s41, 0xf4292244);
		 II_SIMD_hong_2way(d, a, b, c, x[7], s42, 0x432aff97);
		 II_SIMD_hong_2way(c, d, a, b, x[14], s43, 0xab9423a7);
		 II_SIMD_hong_2way(b, c, d, a, x[5], s44, 0xfc93a039);
		 II_SIMD_hong_2way(a, b, c, d, x[12], s41, 0x655b59c3);
		 II_SIMD_hong_2way(d, a, b, c, x[3], s42, 0x8f0ccc92);
		 II_SIMD_hong_2way(c, d, a, b, x[10], s43, 0xffeff47d);
		 II_SIMD_hong_2way(b, c, d, a, x[1], s44, 0x85845dd1);
		 II_SIMD_hong_2way(a, b, c, d, x[8], s41, 0x6fa87e4f);
		 II_SIMD_hong_2way(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		 II_SIMD_hong_2way(c, d, a, b, x[6], s43, 0xa3014314);
		 II_SIMD_hong_2way(b, c, d, a, x[13], s44, 0x4e0811a1);
		 II_SIMD_hong_2way(a, b, c, d, x[4], s41, 0xf7537e82);
		 II_SIMD_hong_2way(d, a, b, c, x[11], s42, 0xbd3af235);
		 II_SIMD_hong_2way(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		 II_SIMD_hong_2way(b, c, d, a, x[9], s44, 0xeb86d391);
 
		 state[0] = vadd_u32(state[0],a);
		 state[1] = vadd_u32(state[1],b);
		 state[2] = vadd_u32(state[2],c);
		 state[3] = vadd_u32(state[3],d);
	 }
 
	 for (int i = 0; i < 4; i++)
	 {
		state[i] = vreinterpret_u32_u8(vrev32_u8(vreinterpret_u8_u32(state[i])));
	 }
 
	 // 输出最终的hash结果
	 // for (int i1 = 0; i1 < 4; i1 += 1)
	 // {
	 // 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	 // }
	 // cout << endl;
 
	 // 释放动态分配的内存
	 // 实现SIMD并行算法的时候，也请记得及时回收内存！
	 for(int i = 0;i < 2; i++){
		delete paddedMessage[i];
	 }
	 delete[] messageLength;
 }
