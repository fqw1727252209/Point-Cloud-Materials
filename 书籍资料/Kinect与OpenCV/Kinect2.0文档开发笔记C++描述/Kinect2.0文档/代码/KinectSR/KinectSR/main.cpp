#include "stdafx.h"
#include "included.h"




// 应用程序入口

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPWSTR /* lpCmdLine */, int nCmdShow){
    if (SUCCEEDED(CoInitialize(nullptr)))
    {
        AllocConsole();
        FILE*m_new_stdout_file = nullptr;
        freopen_s(&m_new_stdout_file, "CONOUT$", "w+t", stdout);
        {
            ThisApp app;
            system("pause");
            app.Exit();
        }
        CoUninitialize();
        fclose(m_new_stdout_file);
        FreeConsole();
    }
	return 0;
}



// 语法文件
WCHAR* ThisApp::s_GrammarFileName = L"Grammar.xml";