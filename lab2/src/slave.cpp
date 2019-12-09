#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <mpi.h>
#include <unistd.h>
#include "consts.cpp"
#include "asyncRequest.cpp"
#include "consoleLock.cpp"

using namespace std;

class Slave {
private:
    int rank;
    int size;
    int slaves_rank;
    int slaves_size;
    int recipient_amount;

    int end_jobs_send_flag = 0;
    int end_jobs_ack_flag = 0;

    int eof_flag = 0, error_flag = 0;

    bool ended_all_jobs = false;
    bool ended_all_jobs_ack = false;

    int input_message_flag = 0;
    queue<string*> message_queue;
    MPI_Request master_request = MPI_REQUEST_NULL;
    MPI_Request error_request = MPI_REQUEST_NULL;
    MPI_Request eof_request = MPI_REQUEST_NULL;
    MPI_Comm* slaves_comm;

public:
    Slave(int rank, int size): 
        rank(rank),
        size(size)
    {
        slaves_comm = (MPI_Comm*)malloc(sizeof(MPI_Comm));
        slaves_rank = rank - 1;
        slaves_size = size - 1;
        recipient_amount = slaves_size - 1;
    };

    int run() {
        MPI_Comm_split(MPI_COMM_WORLD, Consts::SLAVE_COLOR, slaves_rank, slaves_comm);

        int signal = 0;

        MPI_Request end_jobs_request = MPI_REQUEST_NULL;
        MPI_Request end_jobs_ack_request = MPI_REQUEST_NULL;

        MPI_Status master_status;

        ConsoleLock* lock = new ConsoleLock(slaves_rank, slaves_size, slaves_comm);
        lock->init();

        MPI_Irecv(&signal, 1, MPI_INT, Consts::MASTER_RANK, Consts::EOF_TAG, MPI_COMM_WORLD, &eof_request);
        MPI_Irecv(&signal, 1, MPI_INT, Consts::MASTER_RANK, Consts::ERROR_TAG, MPI_COMM_WORLD, &error_request);

        while(true)
        {
            MPI_Iprobe(Consts::MASTER_RANK, Consts::MASTER_TAG, MPI_COMM_WORLD, &input_message_flag, &master_status);
            if (input_message_flag) {
                char* message = accept_message_from_master(&master_status);
                handle_input_message(new string(message));
                lock->request();
            }

            if (!error_flag) {
                MPI_Test(&error_request, &error_flag, MPI_STATUS_IGNORE);
            }
            if (ready_to_terminate()) {
                MPI_Cancel(&eof_request);
                break;
            }

            if (!eof_flag) {
                MPI_Test(&eof_request, &eof_flag, MPI_STATUS_IGNORE);
            }
            if (ready_to_end_all_jobs()) {
                ended_all_jobs = true;
                MPI_Isend(&ended_all_jobs, 1, MPI_CXX_BOOL, Consts::MASTER_RANK, Consts::END_OF_JOBS_TAG, MPI_COMM_WORLD, &end_jobs_request);
                MPI_Irecv(&ended_all_jobs_ack, 1, MPI_CXX_BOOL, Consts::MASTER_RANK, Consts::END_OF_JOBS_TAG, MPI_COMM_WORLD, &end_jobs_ack_request);
            }

            if (ended_all_jobs) {
                MPI_Test(&end_jobs_ack_request, &end_jobs_ack_flag, MPI_STATUS_IGNORE);
                if (end_jobs_ack_flag) {
                    MPI_Test(&end_jobs_request, &end_jobs_send_flag, MPI_STATUS_IGNORE);
                    MPI_Cancel(&error_request);
                    
                    print_debug();
                    
                    break;
                }
            }

            lock->test();
            
            if (lock->lock()) {
                print_message(lock->get_clock());
                lock->exit();
                finalize_print_job();
            }
            
        }

        delete(lock);

        return 0;
    }

private:
    bool wants_output() {
        return !message_queue.empty();
    }

    bool ready_to_end_all_jobs() {
        return eof_flag && !wants_output() && !ended_all_jobs;
    }

    bool ready_to_terminate() {
        return error_flag;
    }

    char* accept_message_from_master(MPI_Status* master_status) {
        int count;
        MPI_Get_count(master_status, MPI_CHAR, &count);
        char* message = (char*)malloc(sizeof(char)*(count+1));
        MPI_Recv(message, count, MPI_CHAR, Consts::MASTER_RANK, Consts::MASTER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        message[count] = '\0';

        return message;
    }

    void handle_input_message(string* message) {
        message_queue.push(message);
        master_request = MPI_REQUEST_NULL;
        // MPI_Request_free(&master_request);
        input_message_flag = 0;
    }

    void print_message(int clock) {
        string* output_message = message_queue.front();
        cout << "Message: '" << *output_message << "', rank: '" << rank << "', clock: '" << clock << "'" << endl;
    }

    void finalize_print_job() {
        string* message = message_queue.front();
        message_queue.pop();
        delete(message);
    }

    void print_debug() {
        cout 
        << "Rank: " << rank << ", "
        << "eof_flag: " << eof_flag << ", "
        << "input_message_flag: " << input_message_flag << ", "
        << "error_flag: " << error_flag << ", "
        << "ended_all_jobs: " << ended_all_jobs << ", "
        << "error_flag: " << error_flag << ", "
        << "end_jobs_ack_flag: " << end_jobs_ack_flag << ", "
        << "message_queue.size(): " << message_queue.size() << ", "
        << endl;
    }
};