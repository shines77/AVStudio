#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>        // For HMODULE, GetModuleHandle(), LoadLibrary(), and so on.

#include <string>

HMODULE load_system_library(const std::string & dll_name);

HMODULE load_system_library(const std::string & dll_name_32,
                            const std::string & dll_name_64);

void free_system_library(HMODULE handle);
