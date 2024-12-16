#pragma once

#include <string>

struct AVInputFormat;
struct AVInputFormat;

class CameraRecoder
{
public:
    CameraRecoder();
    ~CameraRecoder();

    void release();
    void cleanup();

    int init(const std::string & output_file);
    int stop();

protected:
    AVInputFormat * v_input_fmt_;
    AVInputFormat * a_input_fmt_;
};
