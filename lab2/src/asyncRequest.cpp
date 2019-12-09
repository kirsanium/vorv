#pragma once

#include <mpi.h>

class AsyncRequest
{
private:
    /* data */
public:
    int flag;
    MPI_Request* request;
    int* clock;
    AsyncRequest(MPI_Request* request, int* clock);
    ~AsyncRequest();
};

AsyncRequest::AsyncRequest(MPI_Request* request, int* clock): request(request), clock(clock)
{
}

AsyncRequest::~AsyncRequest()
{
    delete(clock);
    delete(request);
}
