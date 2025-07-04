本次实验修改了guessing.cpp内的Generate()函数，guessing_cuda.cu为Generate()函数GPU并行化结果，guessing_cuda_both.cpp为并行化Generate()和Popnext()函数结果

PopNext()包括保留概率降序和去除概率降序两个版本，使用时按需注释掉另一版本即可

其他文件未做修改

服务器编译指令为

    nvcc --std=c++11 main.cpp train.cpp guessing_cuda.cu md5.cpp -o test
    nvcc --std=c++11 main.cpp train.cpp guessing_cuda.cu md5.cpp -o test -O1
    nvcc --std=c++11 main.cpp train.cpp guessing_cuda.cu md5.cpp -o test -O2

    nvcc --std=c++11 main.cpp train.cpp guessing_cuda_both.cu md5.cpp -o test
    nvcc --std=c++11 main.cpp train.cpp guessing_cuda_both.cu md5.cpp -o test -O1
    nvcc --std=c++11 main.cpp train.cpp guessing_cuda_both.cu md5.cpp -o test -O2

执行命令 

    ./test
