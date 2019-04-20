#include "stdafx.h"
#include "included.h"


// 应用程序入口
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
#ifdef _DEBUG
    AllocConsole();
    _cwprintf(L"Battle Control  Online! \n");
#endif

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            ThisApp app(lpCmdLine);
            if (SUCCEEDED(app.Initialize(hInstance, nCmdShow)))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }
#ifdef _DEBUG
    _cwprintf(L"Battle Control Terminated! \n");
    FreeConsole();
#endif
    return 0;
}