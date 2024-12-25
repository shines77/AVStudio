#pragma once

#include "AVConsole.h"

#ifdef _DEBUG
extern av::StdFileLogT console;
#else
extern av::OutputDebugConsoleT console;
#endif
