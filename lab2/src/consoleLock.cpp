#include <queue>
#include <vector>
#include <mpi.h>
#include "consts.cpp"
#include "asyncRequest.cpp"

using namespace std;

class ConsoleLock {
private:
    int rank, size, recipient_amount;
    MPI_Comm* comm;

    int clock = 0;

    int ask_timestamp;
    int ask_flag = 0;
    MPI_Request ask_request = MPI_REQUEST_NULL;
    MPI_Status ask_status;

    queue<vector<AsyncRequest*>*> recv_requests_queue;

    vector<int> deferred_replies;
    vector<AsyncRequest*> send_requests;

public:
    ConsoleLock(int rank, int size, MPI_Comm* comm) : rank(rank), size(size), comm(comm), recipient_amount(size-1)
    {
    }

    void init() {
        MPI_Irecv(&ask_timestamp, 1, MPI_INT, MPI_ANY_SOURCE, Consts::CRITICAL_REQUEST_TAG, *comm, &ask_request);
    }

    void request() {
        send_to_everyone(Consts::CRITICAL_REQUEST_TAG);
        nonblocking_receive_from_everyone(Consts::CRITICAL_RESPONSE_TAG);
    }

    bool lock() {
        if (wants_output()) {
            handle_incoming_replies();

            int replies = get_number_of_replies();

            if (replies == recipient_amount) {
                return true;
            }
        }

        return false;
    }

    void exit() {
        vector<AsyncRequest*>* recv_requests = recv_requests_queue.front();

        for (auto iter = recv_requests->begin(); iter != recv_requests->end(); ++iter) {
            delete(*iter);
        }

        recv_requests_queue.pop();
        delete(recv_requests);
    
        if (!wants_output()) {
            send_all_deferred_messages();
        }
    }

    void test() {
        MPI_Test(&ask_request, &ask_flag, &ask_status);
        while (ask_flag) {
            handle_incoming_ask();
            MPI_Irecv(&ask_timestamp, 1, MPI_INT, MPI_ANY_SOURCE, Consts::CRITICAL_REQUEST_TAG, *comm, &ask_request);
            MPI_Test(&ask_request, &ask_flag, &ask_status);
        }
        
        test_all_send_requests();
    }

    ~ConsoleLock() {
        MPI_Cancel(&ask_request);
    }

    int get_clock() {
        return clock;
    }

private:
    bool wants_output() {
        return !recv_requests_queue.empty();
    }

    void handle_incoming_ask() {
        update_clock(&clock, ask_timestamp);
        if (clock > ask_timestamp || !wants_output()) {
            send_message(&clock, ask_status.MPI_SOURCE, Consts::CRITICAL_RESPONSE_TAG);
        }
        else {
            deferred_replies.push_back(ask_status.MPI_SOURCE);
        }
        ask_flag = 0;
        ask_request = MPI_REQUEST_NULL;
    }

    void handle_incoming_replies() {

        vector<AsyncRequest*>* reply_requests = recv_requests_queue.front();

        for (auto iter = reply_requests->begin(); iter != reply_requests->end(); ++iter) {
            AsyncRequest* request = *iter;
            if (request->flag == 0) {
                MPI_Test(request->request, &(request->flag), MPI_STATUS_IGNORE);
                if (request->flag != 0) {
                    update_clock(&clock, *(request->clock));
                }
            }
        }
    }

    int get_number_of_replies() {
        int replies = 0;

        vector<AsyncRequest*>* reply_requests = recv_requests_queue.front();

        for (auto iter = reply_requests->begin(); iter != reply_requests->end(); ++iter) {
            AsyncRequest* request = *iter;
            if (request->flag != 0) {
                ++replies;
            }
        }

        return replies;
    }    

    void update_clock(int* clock, int timestamp) {
        if (timestamp > *clock)
            *clock = timestamp + 1;
        else
            *clock += 1;
    }

    void send_to_everyone(int tag) {
        for (int i = 0; i < size; ++i) {
            if (i == rank) continue;
            send_message(&clock, i, tag);
        }
    }

    void nonblocking_receive_from_everyone(int tag) {
        vector<AsyncRequest*>* requests_vec = new vector<AsyncRequest*>;

        for (int i = 0; i < size; ++i) {
            if (i == rank) continue;

            AsyncRequest* request = new AsyncRequest(new MPI_Request(), new int());

            MPI_Irecv(request->clock, 1, MPI_INT, i, tag, *comm, request->request);
            requests_vec->push_back(request);
        }

        recv_requests_queue.push(requests_vec);
    }

    void send_all_deferred_messages() {
        if (!deferred_replies.empty()) {
            for (auto iter = deferred_replies.begin(); iter != deferred_replies.end(); ++iter) {
                send_message(&clock, *iter, Consts::CRITICAL_RESPONSE_TAG);
            }
            // cout << "Sent " << deferred_replies.size() << " deferred messages" << endl;
            deferred_replies.clear();
        }
    }

    void send_message(int* clock, int dest, int tag) {
        (*clock)++;
        int* payload = new int(*clock);
        AsyncRequest *send_request = new AsyncRequest(new MPI_Request, payload);
        MPI_Isend(send_request->clock, 1, MPI_INT, dest, tag, *comm, send_request->request);
        send_requests.push_back(send_request);
    }

    void test_all_send_requests() {
        // cout << "Im testing all send requests, rank " << rank << endl;
        auto iter = send_requests.begin();
        while ( iter != send_requests.end() ) {
            AsyncRequest* request = *iter;
            // cout << "Request*: " << request << ", rank: " << rank << ", req->req: "<< request->request << endl;
            MPI_Test(request->request, &(request->flag), MPI_STATUS_IGNORE);
            if (request->flag) {
                delete(request);
                iter = send_requests.erase(iter);
                // cout << "iter after erase: " << *iter << endl;
            }
            else {
                ++iter;
            }
        }
        // cout << "Im finished testing all send requests, rank" << rank << endl;
    }

};