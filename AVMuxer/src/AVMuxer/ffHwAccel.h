#pragma once

#include <string>
#include <vector>
#include <list>

bool is_support_nvenc();

enum HWACCEL_TYPE {
	HWACCEL_TYPE_UNKNOWN,
	HWACCEL_TYPE_NVENC,
	HWACCEL_TYPE_QSV,
	HWACCEL_TYPE_AMF,
	HWACCEL_TYPE_VAAPI
};

class ffHwAccel
{
public:
    ffHwAccel();
    ~ffHwAccel();

    static const bool kIs64Bit = (sizeof(void *) == 8);

    struct HARDWARE_ENCODER {
		HWACCEL_TYPE type;
		std::string  name;
    };
    
    std::vector<std::string>        video_hardware_devices;
    std::vector<HARDWARE_ENCODER>   video_encoders;

protected:
    void init();

    std::size_t get_video_hardware_devices();
    std::size_t get_support_video_encoders();

private:
    //
};
