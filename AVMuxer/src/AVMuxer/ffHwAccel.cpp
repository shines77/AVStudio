
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
    bool is_exists = false;

#ifdef _UNICODE
	std::wstring module_name;
	if (kIs64Bit)
		module_name = L"nvEncodeAPI64.dll";
	else
		module_name = L"nvEncodeAPI.dll";

	HMODULE hnvenc = ::GetModuleHandleW(module_name.c_str());
	if (!hnvenc) {
		hnvenc = ::LoadLibraryW(module_name.c_str());
        is_exists = (hnvenc != nullptr);
        ::FreeModule(hnvenc);
    }
#else
	std::string module_name;
	if (kIs64Bit)
		module_name = "nvEncodeAPI64.dll";
	else
		module_name = "nvEncodeAPI.dll";

	HMODULE hnvenc = ::GetModuleHandleA(module_name.c_str());
	if (!hnvenc) {
		hnvenc = ::LoadLibraryA(module_name.c_str());
        is_exists = (hnvenc != nullptr);
        ::FreeModule(hnvenc);
    }
#endif // _UNICODE

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

#if defined(_WIN32) || defined(_WIN64)
	int error = AE_NO;
	auto adapters = d3d_helper::get_adapters(&error);
	if (error != AE_NO || adapters.size() == 0)
		break;

	bool has_device = false;
	for (std::list<IDXGIAdapter *>::iterator itr = adapters.begin(); itr != adapters.end(); itr++) {
		IDXGIOutput *adapter_output = nullptr;
		DXGI_ADAPTER_DESC adapter_desc = { 0 };
		DXGI_OUTPUT_DESC adapter_output_desc = { 0 };

		HRESULT hr = (*itr)->GetDesc(&adapter_desc);
				
		std::string strdesc = utils_string::unicode_ascii(adapter_desc.Description);
		std::transform(strdesc.begin(), strdesc.end(), strdesc.begin(), ::toupper);

		if (SUCCEEDED(hr) && (strdesc.find("NVIDIA") != std::string::npos) && !is_nvenc_blacklist(strdesc)) {
			has_device = true;
			break;
		}
	}

	if(!has_device) break;
			
	if (!is_nvenc_canload()) break;
#else
	/*
	if (!os_dlopen("libnvidia-encode.so.1"))
		break;
		*/
#endif

	do {
		if (avcodec_find_encoder_by_name("nvenc_h264") == nullptr &&
			avcodec_find_encoder_by_name("h264_nvenc") == nullptr)
			break;



		is_support = true;
	} while (0);

	return is_support;
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
