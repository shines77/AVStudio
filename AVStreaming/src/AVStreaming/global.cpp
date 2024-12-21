
#include "stdafx.h"
#include "global.h"

#ifdef NDEBUG
av::StdFileLogT console;
#else
av::OutputDebugConsoleT console;
#endif
