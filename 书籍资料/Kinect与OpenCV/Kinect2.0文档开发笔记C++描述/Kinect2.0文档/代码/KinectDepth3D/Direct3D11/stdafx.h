#pragma once

#define _CRT_SECURE_NO_WARNINGS // M$ 去死
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers



// 是否使用 Direct2D 进行配合
//#define USING_DIRECT2D

// 是否使用 原版DirectWrite 进行配合(需要Direct2D)
//#define USING_DIRECTWRITE




// Windows Header Files:
#include <windows.h>
// DX11
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
// Kinect
#include <Kinect.h>
#include <Kinect.Face.h>
// C++ 
#include <map>
#include <mutex>
#include <cwchar>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <hash_map>
// C RunTime Header Files
#include <malloc.h>
#include <memory.h>
#include <intsafe.h>


// DX
#ifdef USING_DIRECTINPUT
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif
#ifdef USING_DIRECT2D
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d2d1effectauthor.h>
#endif
#ifdef USING_FWDIRECTWRITE
#include "..\\FW1FontWrapper\\Source\\FW1FontWrapper.h"
#endif
#ifdef USING_DIRECTWRITE
#include <dwrite_1.h>
#endif
#ifdef USING_GAMEIME
#include <Ime.h>
#endif


template<class Interface>
inline void SafeRelease(Interface *&pInterfaceToRelease)
{
    if (pInterfaceToRelease != nullptr)
    {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = nullptr;
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


#ifdef _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#ifdef USING_DIRECTINPUT
#pragma comment ( lib, "dinput8.lib" )
#endif

#ifdef USING_DIRECT2D
#pragma comment ( lib, "d2d1.lib" )
#endif


#ifdef USING_DIRECTWRITE
#pragma comment ( lib, "dwrite.lib" )
#endif

#pragma comment ( lib, "dxgi.lib" )
#pragma comment ( lib, "d3d11.lib" )
#pragma comment ( lib, "windowscodecs.lib" )
#pragma comment ( lib, "dwmapi.lib" )
#pragma comment ( lib, "Msdmo.lib" )
#pragma comment ( lib, "dmoguids.lib" )
#pragma comment ( lib, "amstrmid.lib" )
#pragma comment ( lib, "avrt.lib" )
#pragma comment ( lib, "dwrite.lib" )
#pragma comment ( lib, "dxguid.lib")
#pragma comment ( lib, "kinect20.lib" )
#pragma comment ( lib, "Kinect20.Face.lib" )


#define lengthof(a) (sizeof(a)/(sizeof(*(a))))