本次实验只修改了guess内的Generate()函数，如果三种实现方式相对应的修改过的源码都在各自文件夹，文件夹内没有的则与初始代码相同无修改。

Pthread编译指令为

    g++ main.cpp train.cpp gue_pool.cpp md5.cpp -o main -pthread
    
    g++ main.cpp train.cpp gue_pool.cpp md5.cpp -o main -pthread -O1
    
    g++ main.cpp train.cpp gue_pool.cpp md5.cpp -o main -pthread -O2

omp编译指令为

    g++ -fopenmp main.cpp train.cpp gue_omp.cpp md5.cpp -o main
    
    g++ -fopenmp main.cpp train.cpp gue_omp.cpp md5.cpp -o main -O1
    
    g++ -fopenmp main.cpp train.cpp gue_omp.cpp md5.cpp -o main -O2
    
