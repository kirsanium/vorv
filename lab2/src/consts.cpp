#pragma once

class Consts {
public:
    static const int MASTER_RANK = 0;
    static const int MASTER_COLOR = 0;
    static const int SLAVE_COLOR = 1;
    static const int MASTER_TAG = 12345;
    static const int CRITICAL_REQUEST_TAG = 12121;
    static const int CRITICAL_RESPONSE_TAG = 12122;

    static const int EOF_TAG = 129;
    static const int ERROR_TAG = 130;
    static const int END_OF_JOBS_TAG = 131;
};