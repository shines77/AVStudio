#pragma once

#include "system_lib.h"
#include "string_utils.h"

HMODULE load_system_library(const std::string & dll_name)
{
	std::tstring module_name = string_utils::ansi_to_unicode(dll_name);

#ifdef _UNICODE
	HMODULE handle = ::GetModuleHandleW(module_name.c_str());
	if (!handle) {
		handle = ::LoadLibraryW(module_name.c_str());
    }
#else
	HMODULE handle = ::GetModuleHandleA(module_name.c_str());
	if (!handle) {
		handle = ::LoadLibraryA(module_name.c_str());
    }
#endif
    return handle;
}

HMODULE load_system_library(const std::string & dll_name_32,
                            const std::string & dll_name_64)
{
    static const bool kIs64Bit = (sizeof(void *) == 8);

	std::string module_name = (kIs64Bit) ? dll_name_64 : dll_name_32;
    return load_system_library(module_name);
}

void free_system_library(HMODULE handle)
{
    if (handle) {
	    ::FreeModule(handle);
    }
}
