#include <iostream>
#include <fstream>
#include <string>
#include <mpi.h>
#include <queue>
#include "consts.cpp"
#include "slave.cpp"
#include "master.cpp"

using namespace std;

class Main {
private:
    int rank, size;

public:
    int run(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        int result;
        
        if(rank == Consts::MASTER_RANK)
        {
            Master master = Master(rank, size, argv[1]);
            result = master.run();
        }
        else
        {
            Slave slave = Slave(rank, size);
            result = slave.run();
        }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return result;
    }
};

int main(int argc, char *argv[])
{
	Main task;
    return task.run(argc, argv);
}