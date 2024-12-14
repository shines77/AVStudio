
#include "ffHwAccel.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
}

#include <ctype.h>      // For ::toupper()
#include <algorithm>    // For std::transform()

#include "error_code.h"
#include "string_utils.h"
#include "system_dll.h"
#include "d3d_helper.h"
#include "AVConsole.h"

// 940M is support NVENC
static const std::list<std::string> nvenc_blacklist = {
	"720M", "730M",  "740M",  "745M",  "820M",  "830M",
	"840M", "845M",  "920M",  "930M",  "942M",  "945M",
	"1030", "MX110", "MX130", "MX150", "MX230", "MX250",
	"M520", "M500",  "P500",  "K620M"
};

static std::string get_encoder_name(int hwaccel_type)
{
    switch (hwaccel_type) {
        case HWACCEL_TYPE_UNKNOWN:
            return "";
        case HWACCEL_TYPE_NVENC:
            return "Nvidia.NVENC";
	    case HWACCEL_TYPE_QSV:
            return "Intel.QSV";
	    case HWACCEL_TYPE_AMF:
            return "AMD.AMF";
	    case HWACCEL_TYPE_VAAPI:
            return "FFmpeg.Vaapi";
        default:
            return "";
    }
}

static bool is_nvenc_blacklist(std::string desc)
{
	for (auto itr = nvenc_blacklist.begin(); itr != nvenc_blacklist.end(); itr++) {
		if (desc.find((*itr).c_str()) != std::string::npos)
			return true;
	}

	return false;
}

static bool is_nvenc_dll_exists()
{
    HMODULE hNvEnc = load_system_library("nvEncodeAPI.dll", "nvEncodeAPI64.dll");
    bool is_exists = (hNvEnc != nullptr);
    free_system_library(hNvEnc);

	return is_exists;
}

bool is_support_nvenc()
{
    bool supported = false;

    AVCodec * h264_nvenc = avcodec_find_encoder_by_name("h264_nvenc");
    if (h264_nvenc == nullptr) {
        h264_nvenc = avcodec_find_encoder_by_name("nvenc_h264");
        if (h264_nvenc == nullptr)
            return false;
    }

    HMODULE hDxGi = nullptr;
    do {
#if defined(_WIN32) || defined(_WIN64)
        std::list<IDXGIAdapter *> adapters;
	    int error = d3d_helper::get_adapters(adapters, &hDxGi);
	    if (error != E_NO_ERROR || adapters.size() == 0) {
		    break;
        }

	    bool has_device = false;
	    for (auto iter = adapters.begin(); iter != adapters.end(); ++iter) {
		    IDXGIOutput * adapter_output = nullptr;
		    DXGI_ADAPTER_DESC adapter_desc = { 0 };
		    DXGI_OUTPUT_DESC adapter_output_desc = { 0 };

		    HRESULT hr = (*iter)->GetDesc(&adapter_desc);
				
		    std::string str_desc = string_utils::unicode_to_ansi(adapter_desc.Description);
		    std::transform(str_desc.begin(), str_desc.end(), str_desc.begin(), ::toupper);

		    if (SUCCEEDED(hr) && (str_desc.find("NVIDIA") != std::string::npos) &&
                                 !is_nvenc_blacklist(str_desc)) {
			    has_device = true;
			    break;
		    }
	    }
    
	    if (!has_device)
            break;
			
	    if (!is_nvenc_dll_exists())
            break;
#else
	    /*
	    if (!os_dlopen("libnvidia-encode.so.1")) {
		    break;
        }
        */
#endif
        supported = true;
    } while (0);

    if (hDxGi) {
        free_system_library(hDxGi);
    }

	return supported;
}

bool is_support_qsv()
{
    // TODO:
    return false;
}

bool is_support_amf()
{
    // TODO:
    return false;
}

bool is_support_vaapi()
{
    // TODO:
    return false;
}

ffHwAccel::ffHwAccel()
{
    //init();
}

ffHwAccel::~ffHwAccel()
{
    //
}

std::size_t ffHwAccel::get_video_hardware_devices()
{
	video_hardware_devices.clear();

    AVHWDeviceType result = av_hwdevice_iterate_types(AV_HWDEVICE_TYPE_NONE);
	while (result != AV_HWDEVICE_TYPE_NONE) {
        std::string device_name = av_hwdevice_get_type_name(result);
		video_hardware_devices.push_back(device_name);
		console.debug("hwdevice: %s", device_name.c_str());
        result = av_hwdevice_iterate_types(result);
	}
		
	AVCodec * h264_nvenc = avcodec_find_encoder_by_name("h264_nvenc");
	if (h264_nvenc == nullptr) {
		h264_nvenc = avcodec_find_encoder_by_name("nvenc_h264");
    }

	if (h264_nvenc) {
		console.debug("nvenc support");
    }

	AVCodec * vaapi = avcodec_find_encoder_by_name("h264_qsv");
	if (vaapi) {
		console.debug("qsv support");
    }

	return video_hardware_devices.size();
}

std::size_t ffHwAccel::get_support_video_encoders()
{
	HARDWARE_ENCODER encoder;
    video_encoders.clear();

	encoder.type = HWACCEL_TYPE_NVENC;
	if (is_support_nvenc()) {
        std::string encoder_name = get_encoder_name(encoder.type);
        if (!encoder_name.empty()) {
            encoder.name = encoder_name;
		    video_encoders.push_back(encoder);
        }
	}

	encoder.type = HWACCEL_TYPE_QSV;
	if (is_support_qsv()) {
        std::string encoder_name = get_encoder_name(encoder.type);
        if (!encoder_name.empty()) {
            encoder.name = encoder_name;
		    video_encoders.push_back(encoder);
        }
	}

	encoder.type = HWACCEL_TYPE_AMF;
	if (is_support_amf()) {
        std::string encoder_name = get_encoder_name(encoder.type);
        if (!encoder_name.empty()) {
            encoder.name = encoder_name;
		    video_encoders.push_back(encoder);
        }
	}

	encoder.type = HWACCEL_TYPE_VAAPI;
	if (is_support_vaapi()) {
        std::string encoder_name = get_encoder_name(encoder.type);
        if (!encoder_name.empty()) {
            encoder.name = encoder_name;
		    video_encoders.push_back(encoder);
        }
	}

    return video_encoders.size();
}

void ffHwAccel::init()
{
    get_video_hardware_devices();
    get_support_video_encoders();
}
