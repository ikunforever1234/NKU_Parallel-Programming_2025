#include "PCFG.h"
#include <pthread.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
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

void PriorityQueue::PopNext()
{

    // 对优先队列最前面的PT，首先利用这个PT生成一系列猜测
    Generate(priority.front());

    // 然后需要根据即将出队的PT，生成一系列新的PT
    vector<PT> new_pts = priority.front().NewPTs();
    for (PT pt : new_pts)
    {
        // 计算概率
        CalProb(pt);
        // 接下来的这个循环，作用是根据概率，将新的PT插入到优先队列中
        for (auto iter = priority.begin(); iter != priority.end(); iter++)
        {
            // 对于非队首和队尾的特殊情况
            if (iter != priority.end() - 1 && iter != priority.begin())
            {
                // 判定概率
                if (pt.prob <= iter->prob && pt.prob > (iter + 1)->prob)
                {
                    priority.emplace(iter + 1, pt);
                    break;
                }
            }
            if (iter == priority.end() - 1)
            {
                priority.emplace_back(pt);
                break;
            }
            if (iter == priority.begin() && iter->prob < pt.prob)
            {
                priority.emplace(iter, pt);
                break;
            }
        }
    }

    // 现在队首的PT善后工作已经结束，将其出队（删除）
    priority.erase(priority.begin());
}

// 线程数据结构（如果还没定义的话）
struct PopNextThreadData {
    PriorityQueue* pq;
    vector<PT>* new_pts;
    int start_idx;
    int end_idx;
    pthread_mutex_t* mutex;
};

// 线程工作函数
void* CalProbWorker(void* arg) {
    PopNextThreadData* data = (PopNextThreadData*)arg;
    
    // 处理分配给该线程的PT范围
    for (int i = data->start_idx; i < data->end_idx; i++) {
        data->pq->CalProb((*data->new_pts)[i]);
    }
    
    return NULL;
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


// 这个函数是PCFG并行化算法的主要载体
// 包含现有的实现...

// 单segment情况下的线程工作函数
void* SingleSegmentWorker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    PriorityQueue* pq = data->pq;
    segment* a = data->seg;
    vector<string> local_guesses;  // 本地存储猜测，减少锁竞争
    int local_count = 0;

    // 处理分配给该线程的范围
    for (int i = data->start_idx; i < data->end_idx; i++) {
        string guess = a->ordered_values[i];
        local_guesses.emplace_back(guess);
        local_count++;
    }

    // 获取锁并更新共享数据
    pthread_mutex_lock(data->mutex);
    pq->guesses.insert(pq->guesses.end(), local_guesses.begin(), local_guesses.end());
    pq->total_guesses += local_count;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

// 多segment情况下的线程工作函数
void* MultiSegmentWorker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    PriorityQueue* pq = data->pq;
    segment* a = data->seg;
    string base_guess = data->base_guess;
    vector<string> local_guesses;  // 本地存储猜测，减少锁竞争
    int local_count = 0;

    // 处理分配给该线程的范围
    for (int i = data->start_idx; i < data->end_idx; i++) {
        string temp = base_guess + a->ordered_values[i];
        local_guesses.emplace_back(temp);
        local_count++;
    }

    // 获取锁并更新共享数据
    pthread_mutex_lock(data->mutex);
    pq->guesses.insert(pq->guesses.end(), local_guesses.begin(), local_guesses.end());
    pq->total_guesses += local_count;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

// 并行化的Generate函数
void PriorityQueue::Generate(PT pt) {
    // 计算PT的概率，这里主要是给PT的概率进行初始化
    CalProb(pt);

    // 使用类成员变量 num_threads 作为线程数量
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // 对于只有一个segment的PT
    if (pt.content.size() == 1) {
        // 指向最后一个segment的指针
        segment *a;
        // 在模型中定位到这个segment
        if (pt.content[0].type == 1) {
            a = &m.letters[m.FindLetter(pt.content[0])];
        } else if (pt.content[0].type == 2) {
            a = &m.digits[m.FindDigit(pt.content[0])];
        } else if (pt.content[0].type == 3) {
            a = &m.symbols[m.FindSymbol(pt.content[0])];
        }
        
        // 计算每个线程处理的元素数量
        int total_elements = pt.max_indices[0];
        int elements_per_thread = ceil((double)total_elements / num_threads);
        
        // 创建并启动线程
        for (int t = 0; t < num_threads; t++) {
            thread_data[t].pq = this;
            thread_data[t].pt = pt;
            thread_data[t].seg = a;
            thread_data[t].start_idx = t * elements_per_thread;
            thread_data[t].end_idx = min((t + 1) * elements_per_thread, total_elements);
            thread_data[t].mutex = &mutex;
            
            if (thread_data[t].start_idx >= thread_data[t].end_idx) {
                continue;  // 跳过不需要处理元素的线程
            }
            
            pthread_create(&threads[t], NULL, SingleSegmentWorker, (void*)&thread_data[t]);
        }
        
        // 等待所有线程完成
        for (int t = 0; t < num_threads; t++) {
            if (thread_data[t].start_idx < thread_data[t].end_idx) {
                pthread_join(threads[t], NULL);
            }
        }
    } else {
        // 处理多个segment的PT
        string guess;
        int seg_idx = 0;
        
        // 为前几个segment赋值
        for (int idx : pt.curr_indices) {
            if (pt.content[seg_idx].type == 1) {
                guess += m.letters[m.FindLetter(pt.content[seg_idx])].ordered_values[idx];
            } else if (pt.content[seg_idx].type == 2) {
                guess += m.digits[m.FindDigit(pt.content[seg_idx])].ordered_values[idx];
            } else if (pt.content[seg_idx].type == 3) {
                guess += m.symbols[m.FindSymbol(pt.content[seg_idx])].ordered_values[idx];
            }
            seg_idx += 1;
            if (seg_idx == pt.content.size() - 1) {
                break;
            }
        }

        // 指向最后一个segment的指针
        segment *a;
        if (pt.content[pt.content.size() - 1].type == 1) {
            a = &m.letters[m.FindLetter(pt.content[pt.content.size() - 1])];
        } else if (pt.content[pt.content.size() - 1].type == 2) {
            a = &m.digits[m.FindDigit(pt.content[pt.content.size() - 1])];
        } else if (pt.content[pt.content.size() - 1].type == 3) {
            a = &m.symbols[m.FindSymbol(pt.content[pt.content.size() - 1])];
        }
        
        // 计算每个线程处理的元素数量
        int total_elements = pt.max_indices[pt.content.size() - 1];
        int elements_per_thread = ceil((double)total_elements / num_threads);
        
        // 创建并启动线程
        for (int t = 0; t < num_threads; t++) {
            thread_data[t].pq = this;
            thread_data[t].pt = pt;
            thread_data[t].seg = a;
            thread_data[t].base_guess = guess;
            thread_data[t].start_idx = t * elements_per_thread;
            thread_data[t].end_idx = min((t + 1) * elements_per_thread, total_elements);
            thread_data[t].mutex = &mutex;
            
            if (thread_data[t].start_idx >= thread_data[t].end_idx) {
                continue;  // 跳过不需要处理元素的线程
            }
            
            pthread_create(&threads[t], NULL, MultiSegmentWorker, (void*)&thread_data[t]);
        }
        
        // 等待所有线程完成
        for (int t = 0; t < num_threads; t++) {
            if (thread_data[t].start_idx < thread_data[t].end_idx) {
                pthread_join(threads[t], NULL);
            }
        }
    }
    
    // 清理互斥锁
    pthread_mutex_destroy(&mutex);
}
