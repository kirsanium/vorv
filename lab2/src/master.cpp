#include <iostream>
#include <fstream>
#include <string>
#include <mpi.h>
#include "consts.cpp"

#include <unistd.h>

using namespace std;

class Master {
private:
    int rank;
    int size;
    string input_filename;
    MPI_Comm* root_comm;
    ifstream input_file;

public:
    Master(int rank, int size, string input_filename): 
        rank(rank),
        size(size),
        input_filename(input_filename)
    {
        root_comm = (MPI_Comm*)malloc(sizeof(MPI_Comm));
    };

    int run() {
        MPI_Comm_split(MPI_COMM_WORLD, Consts::MASTER_COLOR, Consts::MASTER_RANK, root_comm);
        
        input_file = ifstream(input_filename);

        if(!input_file.is_open())
        {
            cout << "Error: Cannot read file " << input_filename << "!" << endl;
            return shut_down_all_processes(0);
        }

        string first_line;
        int processes_needed;

        getline(input_file, first_line);
        processes_needed = stoi(first_line);
        if(size != processes_needed + 1)
        {
            cout << "Error: " << processes_needed + 1 << " processes are needed to run! But this MPI has " << size << " processes" << endl;
            input_file.close();
            return shut_down_all_processes(0);
        }
        while(getline(input_file, first_line))
        {
            int space_pos = (int)first_line.find(" ");
            string process_number_entry = first_line.substr(0, space_pos);
            int process_num = stoi(process_number_entry);
            
            string message = first_line.substr(space_pos+1, first_line.length()-space_pos-1);
            // cout << "Master sends message: '" << message << "', c_str: '" << message.c_str() << "', length: " << message.length() << endl;  
            MPI_Send(message.c_str(), message.length(), MPI_CHAR, process_num, Consts::MASTER_TAG, MPI_COMM_WORLD);
        }          


        input_file.close();
        finalize_communication();

        for (int i = 1; i < size; ++i) {
            bool ended_all_jobs = false;
            MPI_Recv(&ended_all_jobs, 1, MPI_CXX_BOOL, i, Consts::END_OF_JOBS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        for (int i = 1; i < size; ++i) {
            bool ended_all_jobs = true;
            MPI_Send(&ended_all_jobs, 1, MPI_CXX_BOOL, i, Consts::END_OF_JOBS_TAG, MPI_COMM_WORLD);
        }

        return 0;
    }

    int shut_down_all_processes(int signal)
    {
        for(int i=0; i<size; i++)
        {
            if (i == Consts::MASTER_RANK) continue;
            MPI_Send(&signal, 1, MPI_INT, i, Consts::ERROR_TAG, MPI_COMM_WORLD);
        }
        return signal;
    }

    int finalize_communication()
    {
        int signal = 0;
        for(int i=0; i<size; i++)
        {
            if (i == Consts::MASTER_RANK) continue;
            MPI_Send(&signal, 1, MPI_INT, i, Consts::EOF_TAG, MPI_COMM_WORLD);
        }
        return 0;
    }
};