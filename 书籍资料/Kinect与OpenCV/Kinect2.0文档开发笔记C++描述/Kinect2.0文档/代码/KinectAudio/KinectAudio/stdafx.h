#pragma once

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <Kinect.h>
#include <d2d1.h>
// C++ 
#include <string>
#include <thread>
#include <map>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>

#define _USE_MATH_DEFINES

#include <math.h>

#include <strsafe.h>

#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include <new> // std::nothrow


template<class Interface>
inline void SafeRelease(Interface *&pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#if defined( DEBUG ) || defined( _DEBUG )
#define MYTRACE(a) _cwprintf(a)
#else
#define MYTRACE(a)
#endif

#pragma comment ( lib, "d2d1.lib" )
#pragma comment ( lib, "windowscodecs.lib" )
#pragma comment ( lib, "dwmapi.lib" )
#pragma comment ( lib, "Msdmo.lib" )
#pragma comment ( lib, "dmoguids.lib" )
#pragma comment ( lib, "amstrmid.lib" )
#pragma comment ( lib, "avrt.lib" )
#pragma comment ( lib, "dwrite.lib" )
#pragma comment ( lib, "kinect20.lib" )
