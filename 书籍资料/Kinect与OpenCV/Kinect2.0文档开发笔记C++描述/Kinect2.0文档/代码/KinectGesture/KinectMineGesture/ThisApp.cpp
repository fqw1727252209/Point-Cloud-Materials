#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) sizeof(a)/sizeof(*a)

// ThisApp构造函数
ThisApp::ThisApp(){

}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 销毁事件
    if (m_hColorFrameArrived && m_pColorFrameReader){
        m_pColorFrameReader->UnsubscribeFrameArrived(m_hColorFrameArrived);
        m_hColorFrameArrived = 0;
    }
    // 释放ColorFrameReader
    SafeRelease(m_pColorFrameReader);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    SafeRelease(m_pKinect);
}

// 初始化
HRESULT ThisApp::Initialize(HINSTANCE hInstance, int nCmdShow){
	HRESULT hr = E_FAIL;
	if (SUCCEEDED(static_cast<HRESULT>(m_ImagaRenderer)))
	{
		//register window class
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = ThisApp::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = L"Direct2DTemplate";
		wcex.hIcon = nullptr;
		// 注册窗口
		RegisterClassEx(&wcex);
		// 创建窗口
		RECT window_rect = { 0, 0, WNDWIDTH, WNDHEIGHT };
		DWORD window_style = WS_CAPTION | WS_VISIBLE | WS_POPUP | WS_SYSMENU |	WS_MINIMIZEBOX;
		AdjustWindowRect(&window_rect, window_style, FALSE);
		AdjustWindowRect(&window_rect, window_style, FALSE);
		window_rect.right -= window_rect.left;
		window_rect.bottom -= window_rect.top;
		window_rect.left = (GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
		window_rect.top = (GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;

		m_hwnd = CreateWindowExW(0, wcex.lpszClassName, TITLE, window_style,
			window_rect.left, window_rect.top, window_rect.right, window_rect.bottom, 0, 0, hInstance, this);
		hr = m_hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{	
			// 设置窗口句柄
			m_ImagaRenderer.SetHwnd(m_hwnd);
			// 显示窗口
			ShowWindow(m_hwnd, nCmdShow);
			UpdateWindow(m_hwnd);
		}
	}

	return hr;
}



// 消息循环
void ThisApp::RunMessageLoop()
{
	MSG msg;
    HANDLE events[] = { reinterpret_cast<HANDLE>(m_hColorFrameArrived) };
    while (true){
        // 消息处理
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        // 设置事件
        // 事件0: 彩色临帧事件
        events[0] = reinterpret_cast<HANDLE>(m_hColorFrameArrived);
        // 检查事件
        switch (MsgWaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
            // events[0]
        case WAIT_OBJECT_0 + 0:
            this->check_color_frame();
            break;
        default:
            break;
        }
        // 退出
        if (msg.message == WM_QUIT){
            break;
        }
    }
}


// 窗口过程函数
LRESULT CALLBACK ThisApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		ThisApp *pOurApp = (ThisApp *)pcs->lpCreateParams;

		::SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			PtrToUlong(pOurApp)
			);

        // 并初始化Kinect
        if (FAILED(pOurApp->init_kinect())){
            ::MessageBoxW(hwnd, L"初始化Kinect v2失败", L"真的很遗憾", MB_ICONERROR);
        }
		result = 1;
	}
	else
	{
		ThisApp *pOurApp = reinterpret_cast<ThisApp *>(static_cast<LONG_PTR>(
			::GetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA
			)));

		bool wasHandled = false;
		if (pOurApp)
		{
			switch (message)
			{
			case WM_DISPLAYCHANGE:
				InvalidateRect(hwnd, NULL, FALSE);
				result = 0;
				wasHandled = true;
				break;
            case WM_MOUSEWHEEL:
            
                pOurApp->m_ImagaRenderer.matrix._11 += 0.05f * static_cast<float>(static_cast<short>(HIWORD(wParam))) 
                    / static_cast<float>(WHEEL_DELTA);
                    pOurApp->m_ImagaRenderer.matrix._22 = pOurApp->m_ImagaRenderer.matrix._11;
                pOurApp->m_ImagaRenderer.OnRender();
            
                break;
			case WM_PAINT:
				pOurApp->m_ImagaRenderer.OnRender();
				break;
			case WM_CLOSE:
				// 将收尾操作(如结束全部子线程)放在这里
				DestroyWindow(hwnd);
				result = 1;
				wasHandled = true;
				break;
			case WM_DESTROY:
				PostQuitMessage(0);
				result = 1;
				wasHandled = true;
				break;
			}
		}

		if (!wasHandled)
		{
			result = DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	return result;
}


// 初始化Kinect
HRESULT ThisApp::init_kinect(){
    IColorFrameSource* pColorFrameSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 获取彩色帧源(ColorFrameSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_ColorFrameSource(&pColorFrameSource);
    }
    // 再获取彩色帧读取器
    if (SUCCEEDED(hr)){
#ifdef _DEBUG
        // 检查是否已存在
        if (m_pColorFrameReader)
            ::MessageBoxW(m_hwnd, L"成员变量:m_pColorFrameReader 值已存在", L"<ThisApp::init_kinect>", MB_ICONERROR);
#endif
        hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
#ifdef _DEBUG
        // 检查是否已存在
        if (m_hColorFrameArrived)
            ::MessageBoxW(m_hwnd, L"成员变量:m_hColorFrameArrived 值已存在", L"<ThisApp::init_kinect>", MB_ICONERROR);
#endif
        m_pColorFrameReader->SubscribeFrameArrived(&m_hColorFrameArrived);
    }
    SafeRelease(pColorFrameSource);
    return hr;
}


// 检查彩色帧
void ThisApp::check_color_frame(){
    if (!m_pColorFrameReader) return;
#ifdef _DEBUG
    static int frame_num = 0;
    ++frame_num; 
    _cwprintf(L"<ThisApp::check_color_frame>Frame@%8d ", frame_num);
#endif 
    // 彩色临帧事件参数
    IColorFrameArrivedEventArgs* pArgs = nullptr;
    // 彩色帧引用
    IColorFrameReference* pCFrameRef = nullptr;
    // 彩色帧
    IColorFrame* pColorFrame = nullptr;
    // 帧描述
    IFrameDescription* pFrameDescription = nullptr;
    // 彩色帧宽度数据
    int width = 0;
    // 彩色帧高度数据
    int height = 0;
    // 帧格式
    ColorImageFormat imageFormat = ColorImageFormat_None;
    // 帧缓存大小
    UINT nBufferSize = 0;
    // 帧缓存
    RGBQUAD *pBuffer = nullptr;

    // 获取参数
    HRESULT hr = m_pColorFrameReader->GetFrameArrivedEventData(m_hColorFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pCFrameRef);
    }
    // 获取彩色帧
    if (SUCCEEDED(hr)) {
        hr = pCFrameRef->AcquireFrame(&pColorFrame);
    }
    // 获取帧描述
    if (SUCCEEDED(hr)) {
        hr = pColorFrame->get_FrameDescription(&pFrameDescription);
    }
    // 获取帧宽度
    if (SUCCEEDED(hr)) {
        hr = pFrameDescription->get_Width(&width);
    }
    // 获取帧高度
    if (SUCCEEDED(hr)) {
        hr = pFrameDescription->get_Height(&height);
    }
    // 获取帧格式
    if (SUCCEEDED(hr)) {
        hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
    }
    // 获取帧真数据
    if (SUCCEEDED(hr)) {
        // 在已经是BGRA的情况下 直接获取源数据
        if (imageFormat == ColorImageFormat_Bgra) {
            hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
        }
        // 负责用自带的方法转换
        else{
            pBuffer = m_ImagaRenderer.GetBuffer();
            nBufferSize = IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(RGBQUAD);
            hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
        }
    }
    // 传输数据
    if (SUCCEEDED(hr)){
        m_ImagaRenderer.WriteBitmapData(pBuffer, width, height);
    }
    // 安全释放
    SafeRelease(pFrameDescription);
    SafeRelease(pColorFrame);
    SafeRelease(pCFrameRef);
    SafeRelease(pArgs);
#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _cwprintf(L" 成功\n");
    else
        _cwprintf(L" 失败\n");
#endif
}