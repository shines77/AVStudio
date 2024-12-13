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

#include "error_code.h"
#include "string_utils.h"
#include "system_dll.h"

typedef HRESULT(WINAPI * DXGI_FUNC_CREATEFACTORY)(REFIID, IDXGIFactory1 **);

class d3d_helper
{
public:
    static int get_adapters(std::list<IDXGIAdapter *> & adapters, bool free_lib = true)
    {
		int error = E_NO_ERROR;

		HMODULE hDxGi = load_system_library("dxgi.dll");
		if (!hDxGi) {
			error = ErrCode(E_D3D_LOAD_FAILED);
			return error;
		}

		do {
			DXGI_FUNC_CREATEFACTORY create_factory = (DXGI_FUNC_CREATEFACTORY)GetProcAddress(hDxGi, "CreateDXGIFactory1");
			if (create_factory == nullptr) {
				error = ErrCode(E_DXGI_GET_PROC_FAILED);
				break;
			}

			IDXGIFactory1 * dxgi_factory = nullptr;
			HRESULT hr = create_factory(__uuidof(IDXGIFactory1), &dxgi_factory);
			if (FAILED(hr)) {
				error = ErrCode(E_DXGI_GET_FACTORY_FAILED);
				break;
			}

			UINT i = 0;
			IDXGIAdapter * adapter = nullptr;
			while (dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
				if (adapter) {
					adapters.push_back(adapter);
                }
				i++;
			}

			dxgi_factory->Release();
		} while (0);

		if (free_lib) {
            free_system_library(hDxGi);
        }

        return error;
    }

private:
    d3d_helper() {}
    ~d3d_helper() {}
};
