#pragma once

#include <stdint.h>
#include <xatomic.h>        // For _Compiler_barrier()
#include <cstdio>
#include <string>

extern "C" {
#include <libavutil/time.h>
}

class StopWatch
{
public:
    StopWatch(bool immediate_mode = false) : start_time_(0), end_time_(0) {
        if (immediate_mode) {
            start();
        }
    }
    ~StopWatch() {
    }

    void reset() {
        _Compiler_barrier();
        start_time_ = 0;
        end_time_ = 0;
    }

    void start() {
        start_time_ = av_gettime_relative();
        _Compiler_barrier();
    }

    void stop() {
        _Compiler_barrier();
        end_time_ = av_gettime_relative();
    }

    void try_stop() {
        if (end_time_ == 0) {
            stop();
        }
    }

    int64_t elapsed_time_stamp() const {
        return (end_time_ - start_time_);
    }

    double elapsed_time_ms() const {
        return ((double)(end_time_ - start_time_) / AV_TIME_BASE * 1000.0);
    }

    double elapsed_time() const {
        return ((double)(end_time_ - start_time_) / AV_TIME_BASE);
    }

    void print_elapsed_time_stamp(const std::string & title) {
        try_stop();
        std::cout << title << " elapsed time: " << elapsed_time_stamp() << std::endl;
        reset();
    }

    void print_elapsed_time_ms(const std::string & title) {
        try_stop();
        std::cout << title << " elapsed time: " << elapsed_time_ms() << " ms" <<std::endl;
        reset();
    }

    void print_elapsed_time(const std::string & title) {
        try_stop();
        std::cout << title << " elapsed time: " << elapsed_time() << " second(s)" <<std::endl;
        reset();
    }

protected:
    int64_t start_time_;
    int64_t end_time_;
};
