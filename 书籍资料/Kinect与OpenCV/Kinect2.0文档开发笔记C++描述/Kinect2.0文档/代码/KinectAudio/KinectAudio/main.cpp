#include "stdafx.h"
#include "included.h"


// 应用程序入口
int main(){
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			ThisApp app;
            system("pause");
            app.Exit();
		}
		CoUninitialize();
	}
	return 0;
}