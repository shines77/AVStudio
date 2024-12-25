
#include "stdafx.h"
#include "global.h"

#ifdef _DEBUG
av::StdFileLogT console;
#else
av::OutputDebugConsoleT console;
#endif
