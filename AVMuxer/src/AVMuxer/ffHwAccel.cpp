
#include "ffHwAccel.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/log.h>
}

#include <ctype.h>      // For ::toupper()
#include <algorithm>    // For std::transform()

#include "error_code.h"
#include "string_utils.h"
#include "system_dll.h"
#include "d3d_helper.h"

static const std::list<std::string> nvenc_blacklist = {
	"720M", "730M",  "740M",  "745M",  "820M",  "830M",
	"840M", "845M",  "920M",  "930M",  "940M",  "945M",
	"1030", "MX110", "MX130", "MX150", "MX230", "MX250",
	"M520", "M500",  "P500",  "K620M"
};

static bool is_nvenc_blacklist(std::string desc) {
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

    AVCodec * nvenc_h264_codec = avcodec_find_encoder_by_name("nvenc_h264");
    if (nvenc_h264_codec == nullptr)
        return false;

    AVCodec * h264_nvenc_codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (h264_nvenc_codec == nullptr)
        return false;

    do {
#if defined(_WIN32) || defined(_WIN64)
        std::list<IDXGIAdapter *> adapters;
	    int error = d3d_helper::get_adapters(adapters);
	    if (error != E_NO_ERROR || adapters.size() == 0) {
		    break;
        }

	    bool has_device = false;
	    for (auto iter = adapters.begin(); iter != adapters.end(); iter++) {
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

	return supported;
}

ffHwAccel::ffHwAccel()
{
    init();
}

ffHwAccel::~ffHwAccel()
{
    //
}

std::size_t ffHwAccel::get_video_hardware_devices()
{
    //
}

std::size_t ffHwAccel::get_support_video_encoders()
{
    //
}

void ffHwAccel::init()
{
    get_video_hardware_devices();
    get_support_video_encoders();
}
