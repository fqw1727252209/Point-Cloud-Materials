#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"

#define lengthof(a) (sizeof(a)/sizeof(*a))


// ThisApp构造函数
ThisApp::ThisApp(){
    m_pColorFrameBuffer = new RGBQUAD[IMAGE_WIDTH * IMAGE_HEIGHT];
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 释放MultiSourceFrameReader
    SafeRelease(m_pColorFrameReader);
    SafeRelease(m_pCoordinateMapper);
    SafeRelease(m_pBodyFrameReader);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    SafeRelease(m_pKinect);
    if (m_pColorFrameBuffer){
        delete[] m_pColorFrameBuffer;
        m_pColorFrameBuffer = nullptr;
    }
}

// 渲染窗口
void ThisApp::Render(ThisApp* pThis){
    // 渲染
    while (true){
        pThis->Update();
        pThis->m_ImagaRenderer.OnRender(1);
        if (pThis->m_bExit) break;
    }
}

// 初始化
HRESULT ThisApp::Initialize(HINSTANCE hInstance, int nCmdShow){
    HRESULT hr = E_FAIL;
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
    DWORD window_style = WS_CAPTION | WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&window_rect, window_style, FALSE);
    window_rect.right -= window_rect.left;
    window_rect.bottom -= window_rect.top;
    window_rect.left = (GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
    window_rect.top = (GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;

    m_hwnd = CreateWindowExW(0, wcex.lpszClassName, TITLE, window_style,
        window_rect.left, window_rect.top, window_rect.right, window_rect.bottom, 0, 0, hInstance, this);
    hr = m_hwnd ? S_OK : E_FAIL;
    // 设置窗口句柄
    if (SUCCEEDED(hr)) {
        hr = m_ImagaRenderer.SetHwnd(m_hwnd);
    }
    // 显示窗口
    if (SUCCEEDED(hr)) {
        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);
        m_threadMSG.std::thread::~thread();
        m_threadMSG.std::thread::thread(Render, this);
    }
	return hr;
}



// 消息循环
void ThisApp::RunMessageLoop(){
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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
		if (pOurApp){
            switch (message)
			{
            case WM_CLOSE:
                // 将收尾操作(如结束全部子线程)放在这里
                pOurApp->m_bExit = TRUE;
                // join
                pOurApp->m_threadMSG.join();
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
    IBodyFrameSource* pBodyFrameSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 打开彩色帧源
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_ColorFrameSource(&pColorFrameSource);
    }
    // 打开彩色帧读取器
    if (SUCCEEDED(hr)) {
        hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
    }
    // 打开骨骼帧源
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_BodyFrameSource(&pBodyFrameSource);
    }
    // 打开骨骼帧读取器
    if (SUCCEEDED(hr)) {
        hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
    }
    // 获取坐标映射器
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_CoordinateMapper(&m_pCoordinateMapper);
    }
    SafeRelease(pBodyFrameSource);
    SafeRelease(pColorFrameSource);
    return hr;
}

// 检查彩色帧
void ThisApp::check_color_frame(){
    IColorFrame* pColorFrame = nullptr;
    IFrameDescription* pColorFrameDescription = nullptr;
    int width = 0, height = 0;
    ColorImageFormat imageFormat = ColorImageFormat_None;
    UINT nBufferSize = 0;
    RGBQUAD *pBuffer = nullptr;

    // 检查是否存在新帧
    HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);

    // 获取彩色信息
    if (SUCCEEDED(hr)){
        hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
    }
    // 获取彩色帧宽度
    if (SUCCEEDED(hr)){
        hr = pColorFrameDescription->get_Width(&width);
    }
    // 获取彩色帧高度
    if (SUCCEEDED(hr)){
        hr = pColorFrameDescription->get_Height(&height);
    }
    // 获取帧格式
    if (SUCCEEDED(hr)) {
        hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
    }
    // 获取彩色帧数据
    if (SUCCEEDED(hr)){
        // 在已经是BGRA的情况下 直接获取源数据
        if (imageFormat == ColorImageFormat_Bgra) {
            hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
        }
        // 负责用自带的方法转换
        else{
            pBuffer = m_pColorFrameBuffer;
            nBufferSize = width * height * sizeof(RGBQUAD);
            hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
        }
    }
    // 传送数据
    if (SUCCEEDED(hr)){
        hr = m_ImagaRenderer.LoadData(pBuffer, width, height);
    }

    SafeRelease(pColorFrame);
    SafeRelease(pColorFrameDescription);
}



// 检查骨骼帧
void ThisApp::check_body_frame(){
    IBodyFrame* pBodyFrame = nullptr;
    IBody*  ppBody[BODY_COUNT] = { 0 };
    // 检查新帧
    HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);

    // 获取骨骼数据
    if (SUCCEEDED(hr)) {
        hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, ppBody);
    }
    // 处理数据
    if (SUCCEEDED(hr)){
        for (int i = 0; i < BODY_COUNT; ++i){
            // 保证数据
            BOOLEAN bTracked = false;
            hr = ppBody[i]->get_IsTracked(&bTracked);
            if (!ppBody[i] || !bTracked){
                m_ImagaRenderer.m_men[i].man = LaughingMan::Offline;
                continue;
            }
            IBody* pNowBody = ppBody[i];
            Joint joints[JointType_Count];
            hr = pNowBody->GetJoints(JointType_Count, joints); 
            if (SUCCEEDED(hr)) {
                m_ImagaRenderer.m_men[i].man = LaughingMan::Online;
                m_ImagaRenderer.m_men[i].update();
                // 坐标映射
                ColorSpacePoint depthPoint = { 0.f, 0.f };
                m_pCoordinateMapper->MapCameraPointToColorSpace(joints[JointType_Head].Position, &depthPoint);
                // 模拟抖动
                m_ImagaRenderer.m_men[i].pos.width = depthPoint.X + 2.f * static_cast<float>((rand() % 4) - 2);
                m_ImagaRenderer.m_men[i].pos.height = depthPoint.Y + 2.f * static_cast<float>((rand() % 4) - 2);
                // 设置远近大小
                m_ImagaRenderer.m_men[i].zoom = 1.f / joints[JointType_Head].Position.Z;
            }
        }

    }
    for (int i = 0; i < BODY_COUNT; ++i)SafeRelease(ppBody[i]);
    SafeRelease(pBodyFrame);
}

// 刷新对象
void ThisApp::Update(){
    // 检查彩色帧
    check_color_frame();
    // 检查骨骼帧
    check_body_frame();
}

