#include "PCFG.h"
#include <mpi.h>
#include <cstring>
#include <algorithm>
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

// 序列化
inline void serialize_PT(const PT& pt, std::vector<int>& int_buf, std::vector<float>& float_buf) {
    // curr_indices
    int_buf.push_back((int)pt.curr_indices.size());
    int_buf.insert(int_buf.end(), pt.curr_indices.begin(), pt.curr_indices.end());
    // max_indices
    int_buf.push_back((int)pt.max_indices.size());
    int_buf.insert(int_buf.end(), pt.max_indices.begin(), pt.max_indices.end());
    // pivot
    int_buf.push_back(pt.pivot);
    // seq_id
    int_buf.push_back(pt.seq_id);
    // preterm_prob, prob
    float_buf.push_back(pt.preterm_prob);
    float_buf.push_back(pt.prob);
}

// 反序列化PT
inline PT deserialize_PT(const std::vector<int>& int_buf, const std::vector<float>& float_buf, size_t& int_pos, size_t& float_pos) {
    PT pt;
    int n = int_buf[int_pos++];
    pt.curr_indices.assign(int_buf.begin() + int_pos, int_buf.begin() + int_pos + n);
    int_pos += n;
    int m = int_buf[int_pos++];
    pt.max_indices.assign(int_buf.begin() + int_pos, int_buf.begin() + int_pos + m);
    int_pos += m;
    pt.pivot = int_buf[int_pos++];
    pt.seq_id = int_buf[int_pos++];
    pt.preterm_prob = float_buf[float_pos++];
    pt.prob = float_buf[float_pos++];
    return pt;
}

void PriorityQueue::PopNext()
{
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    std::vector<PT> curr_priority = priority;
    std::vector<PT> local_new_pts;

    for (size_t i = world_rank; i < curr_priority.size(); i += world_size)
    {
        Generate(curr_priority[i]);
        std::vector<PT> new_pts = curr_priority[i].NewPTs();
        for (PT &pt : new_pts)
        {
            CalProb(pt);
            local_new_pts.push_back(pt);
        }
    }

    std::vector<int> local_int_buf;
    std::vector<float> local_float_buf;
    for (const PT& pt : local_new_pts) {
        serialize_PT(pt, local_int_buf, local_float_buf);
    }
    int local_int_count = (int)local_int_buf.size();
    int local_float_count = (int)local_float_buf.size();

    std::vector<int> all_int_counts(world_size), all_float_counts(world_size);
    MPI_Gather(&local_int_count, 1, MPI_INT, all_int_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_float_count, 1, MPI_INT, all_float_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<int> displs_int(world_size, 0), displs_float(world_size, 0);
    int total_int = 0, total_float = 0;
    if (world_rank == 0) {
        for (int i = 0; i < world_size; ++i) {
            displs_int[i] = total_int;
            total_int += all_int_counts[i];
            displs_float[i] = total_float;
            total_float += all_float_counts[i];
        }
    }
    std::vector<int> all_int_buf(total_int);
    std::vector<float> all_float_buf(total_float);

    MPI_Gatherv(local_int_buf.data(), local_int_count, MPI_INT,
                all_int_buf.data(), all_int_counts.data(), displs_int.data(), MPI_INT,
                0, MPI_COMM_WORLD);
    MPI_Gatherv(local_float_buf.data(), local_float_count, MPI_FLOAT,
                all_float_buf.data(), all_float_counts.data(), displs_float.data(), MPI_FLOAT,
                0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        priority.clear();
        size_t int_pos = 0, float_pos = 0;
        while (int_pos < all_int_buf.size()) {
            PT pt = deserialize_PT(all_int_buf, all_float_buf, int_pos, float_pos);
            priority.push_back(pt);
        }
    }

    int new_size = (world_rank == 0) ? (int)priority.size() : 0;
    MPI_Bcast(&new_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (world_rank != 0) priority.resize(new_size);
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

void PriorityQueue::Generate(PT pt)
{
    // 在本地取得 MPI rank 和 size
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

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
        
        // Multi-thread TODO：
        // 这个for循环就是你需要进行并行化的主要部分了，特别是在多线程&GPU编程任务中
        // 可以看到，这个循环本质上就是把模型中一个segment的所有value，赋值到PT中，形成一系列新的猜测
        // 这个过程是可以高度并行化的
        // 每个进程处理 i = rank, rank + size, rank + 2*size, ...
        for (int i = world_rank; i < pt.max_indices[0]; i += world_size)
        {
            string guess = a->ordered_values[i];
            // cout << guess << endl;
            guesses.emplace_back(guess);
            total_guesses += 1;
        }
    }
    else
    {
        string guess;
        int seg_idx = 0;
        // 这个for循环的作用：给当前PT的所有segment赋予实际的值（最后一个segment除外）
        // segment值根据curr_indices中对应的值加以确定
        // 这个for循环你看不懂也没太大问题，并行算法不涉及这里的加速
        for (int idx : pt.curr_indices)
        {
            if (pt.content[seg_idx].type == 1)
            {
                guess += m.letters[m.FindLetter(pt.content[seg_idx])].ordered_values[idx];
            }
            if (pt.content[seg_idx].type == 2)
            {
                guess += m.digits[m.FindDigit(pt.content[seg_idx])].ordered_values[idx];
            }
            if (pt.content[seg_idx].type == 3)
            {
                guess += m.symbols[m.FindSymbol(pt.content[seg_idx])].ordered_values[idx];
            }
            seg_idx += 1;
            if (seg_idx == pt.content.size() - 1)
            {
                break;
            }
        }

        // 指向最后一个segment的指针，这个指针实际指向模型中的统计数据
        segment *a;
        if (pt.content[pt.content.size() - 1].type == 1)
        {
            a = &m.letters[m.FindLetter(pt.content[pt.content.size() - 1])];
        }
        if (pt.content[pt.content.size() - 1].type == 2)
        {
            a = &m.digits[m.FindDigit(pt.content[pt.content.size() - 1])];
        }
        if (pt.content[pt.content.size() - 1].type == 3)
        {
            a = &m.symbols[m.FindSymbol(pt.content[pt.content.size() - 1])];
        }
        
        // Multi-thread TODO：
        // 这个for循环就是你需要进行并行化的主要部分了，特别是在多线程&GPU编程任务中
        // 可以看到，这个循环本质上就是把模型中一个segment的所有value，赋值到PT中，形成一系列新的猜测
        // 这个过程是可以高度并行化的
        // 每个进程处理 i = rank, rank + size, rank + 2*size, ...
        for (int i = world_rank; i < pt.max_indices[pt.content.size() - 1]; i += world_size)
        {
            string temp = guess + a->ordered_values[i];
            // cout << temp << endl;
            guesses.emplace_back(temp);
            total_guesses += 1;
        }
    }
}
