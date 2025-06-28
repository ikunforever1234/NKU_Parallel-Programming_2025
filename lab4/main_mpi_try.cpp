#include "PCFG.h"
#include <mpi.h>
#include <fstream>
#include "md5.h"
#include <iomanip>
#include <iostream>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    double time_hash  = 0.0; 
    double time_guess = 0.0;
    double time_train = 0.0;

    PriorityQueue q;
    
    double t0 = MPI_Wtime();
    q.m.train("./input/Rockyou-singleLined-full.txt");
    q.m.order();
    double t1 = MPI_Wtime();
    time_train = t1 - t0;
    if (world_rank == 0) {
        std::cout << "train time: " << time_train << " seconds\n";
    }

    q.init();
    if (world_rank == 0) {
        std::cout << "PQ init done.\n";
    }

    int curr_num = 0;
    int history  = 0;
    const int LOG_INTERVAL     = 100000;
    const int FLUSH_THRESHOLD  = 1000000;
    const int GENERATE_LIMIT   = 10000000;

    double t_guess_start = MPI_Wtime();

    while (!q.priority.empty())
    {
        q.PopNext();

        q.total_guesses = q.guesses.size();

        if (q.total_guesses - curr_num >= LOG_INTERVAL)
        {
            if (world_rank == 0) {
                std::cout << "Guesses generated: " << (history + q.total_guesses) << "\n";
            }
            curr_num = q.total_guesses;

            if (history + q.total_guesses > GENERATE_LIMIT)
            {
                double t_guess_end = MPI_Wtime();
                time_guess = (t_guess_end - t_guess_start) - time_hash;
                if (world_rank == 0) {
                    std::cout << "guess time: " << time_guess << " seconds\n";
                    std::cout << "hash time:  " << time_hash  << " seconds\n";
                    std::cout << "train time: " << time_train << " seconds\n";
                }
                break;
            }
        }

        if (curr_num > FLUSH_THRESHOLD)
        {
            double t_hash_start = MPI_Wtime();
            bit32 state[4];
            for (const std::string &pw : q.guesses)
            {
                MD5Hash(pw, state);
            }
            double t_hash_end = MPI_Wtime();
            time_hash += (t_hash_end - t_hash_start);

            history += curr_num;
            curr_num = 0;
            q.guesses.clear();
        }
    }

    MPI_Finalize();
    return 0;
}
