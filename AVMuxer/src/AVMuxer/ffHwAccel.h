#pragma once

#include <string>
#include <vector>
#include <list>

class ffHwAccel
{
public:
    ffHwAccel();
    ~ffHwAccel();

    static const bool kIs64Bit = (sizeof(void *) == 8);
    
    std::vector<std::string> video_hardware_devices;
    std::vector<std::string> video_encoders;

protected:
    void init();

    std::size_t get_video_hardware_devices();
    std::size_t get_support_video_encoders();

private:
    //
};
