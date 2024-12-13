#pragma once

// Maybe you can use <winsdkver.h> to find Windows SDK version also.
#include <sdkddkver.h>

#if (defined(_WIN32_WINNT) && (_WIN32_WINNT == 0x0603)) || \
    (defined(WINVER) && (WINVER == 0x0603))
  // Windows SDK 8.1, support dxgi1_2.h and dxgi1_3.h
  #include <dxgi1_2.h>
#elif (defined(_WIN32_WINNT) && (_WIN32_WINNT == 0x0A00)) || \
      (defined(WINVER) && (WINVER == 0x0A00))
  // Windows SDK 10.x, support from dxgi1_2.h to dxgi1_6.h
  #include <dxgi1_6.h>
#else
  // Try to include <dxgi1_1.h>
  #include <dxgi1_1.h>
#endif // Windows SDK Version

#include <list>

#include "string_utils.h"

HMODULE load_system_library(const std::string & dll_name)
{
	std::tstring module_name = string_utils::ansi_to_unicode(dll_name);

#ifdef _UNICODE
	HMODULE hModule = ::GetModuleHandleW(module_name.c_str());
	if (!hModule) {
		hModule = ::LoadLibraryW(module_name.c_str());
    }
#else
	HMODULE hModule = ::GetModuleHandleA(module_name.c_str());
	if (!hModule) {
		hModule = ::LoadLibraryA(module_name.c_str());
    }
#endif
    return hModule;
}

HMODULE load_system_library(const std::string & dll_name_32, const std::string & dll_name_64)
{
    static const bool kIs64Bit = (sizeof(void *) == 8);

	std::string module_name = (kIs64Bit) ? dll_name_64 : dll_name_32;
    return load_system_library(module_name);
}

class d3d_helper
{
    static int get_adapters(std::list<IDXGIAdapter *> & list, bool free_lib = false) {
		std::list<IDXGIAdapter *> adapters;

		int error = S_OK;

		HMODULE hdxgi = load_system_library("dxgi.dll");
		if (!hdxgi) {
			error = AE_D3D_LOAD_FAILED;
			return adapters;
		}

		do {
			DXGI_FUNC_CREATEFACTORY create_factory = nullptr;
			create_factory = (DXGI_FUNC_CREATEFACTORY)GetProcAddress(hdxgi, "CreateDXGIFactory1");
			if (create_factory == nullptr) {
				error = AE_DXGI_GET_PROC_FAILED;
				break;
			}

			IDXGIFactory1 * dxgi_factory = nullptr;
			HRESULT hr = create_factory(__uuidof(IDXGIFactory1), &dxgi_factory);
			if (FAILED(hr)) {
				error = AE_DXGI_GET_FACTORY_FAILED;
				break;
			}

			unsigned int i = 0;
			IDXGIAdapter *adapter = nullptr;
			while (dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
				if(adapter)
					adapters.push_back(adapter);
				++i;
			}

			dxgi_factory->Release();
		} while (0);

		if (free_lib && hdxgi) {
            free_system_library(hdxgi);
        }

		return adapters;
        return -1;
    }

private:
    d3d_helper() {}
    ~d3d_helper() {}
};
