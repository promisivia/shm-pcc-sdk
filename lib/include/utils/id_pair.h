#pragma once

namespace utils {
    class IDPair {
    public:
        int machine_id;
        int thread_id;

        IDPair(const int& mid, const int& tid) : 
            machine_id(mid), thread_id(tid) {}
    };
}