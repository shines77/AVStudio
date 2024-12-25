#pragma once

#include "AVConsole.h"

#ifdef NDEBUG
extern av::StdFileLogT console;
#else
extern av::OutputDebugConsoleT console;
#endif
