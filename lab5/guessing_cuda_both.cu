#include "PCFG.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
using namespace std;

void PriorityQueue::CalProb(PT &pt)
{
    // 计算PriorityQueue里面一个PT的流程如下：
    // 1. 首先需要计算一个PT本身的概率。例如，L6S1的概率为0.15
    // 2. 需要注意的是，Queue里面的PT不是“纯粹的”PT，而是除了最后一个segment以外，全部被value实例化的PT
    // 3. 所以，对于L6S1而言，其在Queue里面的实际PT可能是123456S1，其中“123456”为L6的一个具体value。
    // 4. 这个时候就需要计算123456在L6中出现的概率了。假设123456在所有L6 segment中的概率为0.1，那么123456S1的概率就是0.1*0.15

    // 计算一个PT本身的概率。后续所有具体segment value的概率，直接累乘在这个初始概率值上
    pt.prob = pt.preterm_prob;

    // index: 标注当前segment在PT中的位置
    int index = 0;


    for (int idx : pt.curr_indices)
    {
        // pt.content[index].PrintSeg();
        if (pt.content[index].type == 1)
        {
            // 下面这行代码的意义：
            // pt.content[index]：目前需要计算概率的segment
            // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
            // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
            // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
            pt.prob *= m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.letters[m.FindLetter(pt.content[index])].total_freq;
            // cout << m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.letters[m.FindLetter(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 2)
        {
            pt.prob *= m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.digits[m.FindDigit(pt.content[index])].total_freq;
            // cout << m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.digits[m.FindDigit(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 3)
        {
            pt.prob *= m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.symbols[m.FindSymbol(pt.content[index])].total_freq;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].total_freq << endl;
        }
        index += 1;
    }
    // cout << pt.prob << endl;
}

void PriorityQueue::init()
{
    // cout << m.ordered_pts.size() << endl;
    // 用所有可能的PT，按概率降序填满整个优先队列
    for (PT pt : m.ordered_pts)
    {
        for (segment seg : pt.content)
        {
            if (seg.type == 1)
            {
                // 下面这行代码的意义：
                // max_indices用来表示PT中各个segment的可能数目。例如，L6S1中，假设模型统计到了100个L6，那么L6对应的最大下标就是99
                // （但由于后面采用了"<"的比较关系，所以其实max_indices[0]=100）
                // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
                // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
                // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
                pt.max_indices.emplace_back(m.letters[m.FindLetter(seg)].ordered_values.size());
            }
            if (seg.type == 2)
            {
                pt.max_indices.emplace_back(m.digits[m.FindDigit(seg)].ordered_values.size());
            }
            if (seg.type == 3)
            {
                pt.max_indices.emplace_back(m.symbols[m.FindSymbol(seg)].ordered_values.size());
            }
        }
        pt.preterm_prob = float(m.preterm_freq[m.FindPT(pt)]) / m.total_preterm;
        // pt.PrintPT();
        // cout << " " << m.preterm_freq[m.FindPT(pt)] << " " << m.total_preterm << " " << pt.preterm_prob << endl;

        // 计算当前pt的概率
        CalProb(pt);
        // 将PT放入优先队列
        priority.emplace_back(pt);
    }
    // cout << "priority size:" << priority.size() << endl;
}


// 这个函数你就算看不懂，对并行算法的实现影响也不大
// 当然如果你想做一个基于多优先队列的并行算法，可能得稍微看一看了
vector<PT> PT::NewPTs()
{
    // 存储生成的新PT
    vector<PT> res;

    // 假如这个PT只有一个segment
    // 那么这个segment的所有value在出队前就已经被遍历完毕，并作为猜测输出
    // 因此，所有这个PT可能对应的口令猜测已经遍历完成，无需生成新的PT
    if (content.size() == 1)
    {
        return res;
    }
    else
    {
        // 最初的pivot值。我们将更改位置下标大于等于这个pivot值的segment的值（最后一个segment除外），并且一次只更改一个segment
        // 上面这句话里是不是有没看懂的地方？接着往下看你应该会更明白
        int init_pivot = pivot;

        // 开始遍历所有位置值大于等于init_pivot值的segment
        // 注意i < curr_indices.size() - 1，也就是除去了最后一个segment（这个segment的赋值预留给并行环节）
        for (int i = pivot; i < curr_indices.size() - 1; i += 1)
        {
            // curr_indices: 标记各segment目前的value在模型里对应的下标
            curr_indices[i] += 1;

            // max_indices：标记各segment在模型中一共有多少个value
            if (curr_indices[i] < max_indices[i])
            {
                // 更新pivot值
                pivot = i;
                res.emplace_back(*this);
            }

            // 这个步骤对于你理解pivot的作用、新PT生成的过程而言，至关重要
            curr_indices[i] -= 1;
        }
        pivot = init_pivot;
        return res;
    }

    return res;
}


#define MAX_VALUE_LEN 32
#define MAX_PTS_BATCH 64
void PriorityQueue::PopNext() 
{
    int B = min((int)priority.size(), MAX_PTS_BATCH);
    vector<PT> batch(priority.begin(), priority.begin() + B);
    priority.erase(priority.begin(), priority.begin() + B);
    
    for (auto &pt : batch) 
    {
        Generate(pt);
    }
    
    for (auto &pt : batch) 
    {
        auto newpts = pt.NewPTs();
        for (auto &n : newpts) 
        {
            CalProb(n);
            auto it = priority.begin();
            while (it != priority.end() && it->prob >= n.prob) 
            {
                ++it;
            }
            priority.insert(it, n);
        }
    }
}

// 全局缓冲
static char* global_valbuf = nullptr;
static int*  global_vallens = nullptr;
static char* global_outbuf = nullptr;

// GPU 常量前缀
__device__ __constant__ char d_prefix[64 * MAX_VALUE_LEN];
__device__ __constant__ int  d_prefix_len;

// 并行 Kernel：拼接前缀 + 最后 segment
__global__ void batchLastSegKernel(
    const char* val_buf, const int* val_lens, int total,
    char* out_buf) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= total) return;
    int plen = d_prefix_len;
    char* tgt = out_buf + idx * MAX_VALUE_LEN * 2;
    __shared__ char shared_prefix[64 * MAX_VALUE_LEN];
    if (threadIdx.x < plen)
        shared_prefix[threadIdx.x] = d_prefix[threadIdx.x];
    __syncthreads();
    for (int i = 0; i < plen; ++i) tgt[i] = shared_prefix[i];
    int len = val_lens[idx];
    char* src = (char*)(val_buf + idx * MAX_VALUE_LEN);
    for (int i = 0; i < len; ++i) tgt[plen + i] = src[i];
    tgt[plen + len] = '\0';
}

void InitGenerateGPUResources(size_t max_values) {
    cudaMalloc(&global_valbuf, max_values * MAX_VALUE_LEN);
    cudaMalloc(&global_vallens, max_values * sizeof(int));
    cudaMalloc(&global_outbuf, max_values * MAX_VALUE_LEN * 2);
}

// 保持原有Generate签名，使用单次GPU调用生成最后segment
void launchLastSegment(const string &prefix,
                      const vector<string> &values,
                      vector<string> &guesses) 
{
    int total = (int)values.size();
    if (!total) 
    {
        return;
    }

    // 如果未初始化全局缓冲，则按当前total大小初始化一次
    if (global_valbuf == nullptr) 
    {
        InitGenerateGPUResources(total);
    }

    // 拷贝前缀到常量内存
    cudaMemcpyToSymbol(d_prefix, prefix.c_str(), prefix.size());
    int prelen = (int)prefix.size();
    cudaMemcpyToSymbol(d_prefix_len, &prelen, sizeof(int));
    
    // 准备值缓冲区
    vector<char> h_valbuf(total * MAX_VALUE_LEN);
    vector<int> h_vallens(total);
    for (int i = 0; i < total; ++i) 
    {
        strncpy(&h_valbuf[i * MAX_VALUE_LEN], values[i].c_str(), MAX_VALUE_LEN);
        h_vallens[i] = values[i].size();
    }
    
    cudaMemcpy(global_valbuf, h_valbuf.data(), total * MAX_VALUE_LEN, cudaMemcpyHostToDevice);
    cudaMemcpy(global_vallens, h_vallens.data(), total * sizeof(int), cudaMemcpyHostToDevice);
    
    // 启动GPU Kernel
    int threads = 256;
    int blocks = (total + threads - 1) / threads;
    batchLastSegKernel<<<blocks, threads>>>(global_valbuf, global_vallens, total, global_outbuf);
    cudaDeviceSynchronize();
    
    // 将结果从GPU拷贝回主机
    vector<char> h_out(total * MAX_VALUE_LEN * 2);
    cudaMemcpy(h_out.data(), global_outbuf, total * MAX_VALUE_LEN * 2, cudaMemcpyDeviceToHost);
    for (int i = 0; i < total; ++i)
    {
        guesses.emplace_back(string(&h_out[i * MAX_VALUE_LEN * 2]));
    }
}


// 这个函数是PCFG并行化算法的主要载体，现已优化为GPU并行版本
// 尽量看懂，然后进行并行实现
void PriorityQueue::Generate(PT pt)
{
    // 计算PT的概率，这里主要是给PT的概率进行初始化
    CalProb(pt);

    // 对于只有一个segment的PT，直接遍历生成其中的所有value即可
    if (pt.content.size() == 1)
    {
        // 指向最后一个segment的指针，这个指针实际指向模型中的统计数据
        segment *a;
        // 在模型中定位到这个segment
        if (pt.content[0].type == 1)
        {
            a = &m.letters[m.FindLetter(pt.content[0])];
        }
        if (pt.content[0].type == 2)
        {
            a = &m.digits[m.FindDigit(pt.content[0])];
        }
        if (pt.content[0].type == 3)
        {
            a = &m.symbols[m.FindSymbol(pt.content[0])];
        }
        
        for (int i = 0; i < pt.max_indices[0]; i += 1)
        {
            string guess = a->ordered_values[i];
            // cout << guess << endl;
            guesses.emplace_back(guess);
            total_guesses += 1;
        }
    }
    else
    {
        string prefix;
        
        // 这个for循环的作用：给当前PT的所有segment赋予实际的值（最后一个segment除外）
        // segment值根据curr_indices中对应的值加以确定
        for (int i = 0; i + 1 < pt.curr_indices.size(); ++i) 
        {
            auto &seg = pt.content[i]; 
            int idx = pt.curr_indices[i];
            if (seg.type == 1) 
            {
                prefix += m.letters[m.FindLetter(seg)].ordered_values[idx];
            }
            else if (seg.type == 2) 
            {
                prefix += m.digits[m.FindDigit(seg)].ordered_values[idx];
            }
            else 
            {
                prefix += m.symbols[m.FindSymbol(seg)].ordered_values[idx];
            }
        }

        // 指向最后一个segment的指针，这个指针实际指向模型中的统计数据
        auto &last = pt.content.back();
        segment *a;
        if (last.type == 1)
        {
            a = &m.letters[m.FindLetter(last)];
        }
        else if (last.type == 2)
        {
            a = &m.digits[m.FindDigit(last)];
        }
        else
        {
            a = &m.symbols[m.FindSymbol(last)];
        }
        
        launchLastSegment(prefix, a->ordered_values, guesses);
        total_guesses += a->ordered_values.size();
    }
}

