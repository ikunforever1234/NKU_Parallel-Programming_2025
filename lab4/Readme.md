本次实验修改了guessing.cpp内的Generate()函数，guessing_mpi_tyr.cpp为Generate()函数MPI并行化结果，guessing_mpi.cpp为并行化Generate()和Popnext()函数结果

同时修改了main函数的计时方法为main_mpi_try.cpp

服务器编译指令为

    mpic++ main_mpi_try.cpp train.cpp guessing_mpi_try.cpp md5.cpp -o main
    mpic++ main_mpi_try.cpp train.cpp guessing_mpi_try.cpp md5.cpp -o main -O1
    mpic++ main_mpi_try.cpp train.cpp guessing_mpi_try.cpp md5.cpp -o main -O2

本地编译指令

    g++ train.cpp md5.cpp main_mpi_try.cpp guessing_mpi_try.cpp -o main -I D:\MPI\Include（替换为你的MPI包Include路径）-L D:\MPI\Lib\x64（替换为你的MPI包x64路径）  -lmsmpi 

    执行命令 mpiexec -n test
