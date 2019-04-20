#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) sizeof(a)/sizeof(*a)

// ThisApp构造函数
ThisApp::ThisApp(){
    m_pColorCoordinates = new ColorSpacePoint[IMAGE_D_WIDTH*IMAGE_D_HEIGHT];
    m_pColorFrameBuffer = new RGBQUAD[IMAGE_C_WIDTH*IMAGE_C_HEIGHT];
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 销毁临帧事件
    if (m_hMultiSourceFrameArrived && m_pMultiSourceFrameReader){
        m_pMultiSourceFrameReader->UnsubscribeMultiSourceFrameArrived(m_hMultiSourceFrameArrived);
        m_hMultiSourceFrameArrived = 0;
    }
    // 释放MultiSourceFrameReader
    SafeRelease(m_pMultiSourceFrameReader);
    // 销毁映射器改变事件
    if (m_hCoordinateMapperChanged && m_pCoordinateMapper){
        m_pCoordinateMapper->UnsubscribeCoordinateMappingChanged(m_hCoordinateMapperChanged);
        m_hCoordinateMapperChanged = 0;
    }
    // 释放映射器
    SafeRelease(m_pCoordinateMapper);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    SafeRelease(m_pKinect);
    // 释放数据
    if (m_pColorCoordinates){
        delete[] m_pColorCoordinates;
        m_pColorCoordinates = nullptr;
    }
    if (m_pColorFrameBuffer){
        delete[] m_pColorFrameBuffer;
        m_pColorFrameBuffer = nullptr;
    }
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
    HANDLE events[] = { 
        reinterpret_cast<HANDLE>(m_hMultiSourceFrameArrived) ,
       // reinterpret_cast<HANDLE>(m_hCoordinateMapperChanged),
    };
    while (true){
        // 消息处理
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        // 设置事件
        // 事件0: 复源临帧事件
        events[0] = reinterpret_cast<HANDLE>(m_hMultiSourceFrameArrived);
        // 事件1: 映射器改变事件
        //events[1] = reinterpret_cast<HANDLE>(m_hCoordinateMapperChanged);
        // 检查事件
        switch (MsgWaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
            // events[0]
        case WAIT_OBJECT_0 + 0:
            this->check_color_frame();
            break;
#if 0
            // events[1]
        case WAIT_OBJECT_0 + 1:
        {
            ICoordinateMappingChangedEventArgs* pArgs;
            m_pCoordinateMapper->GetCoordinateMappingChangedEventData(m_hCoordinateMapperChanged, &pArgs);
            // 目前啥也干不了
            
            // 就释放了
            SafeRelease(pArgs);
        }
#ifdef _DEBUG
            _cwprintf(L"<ThisApp::RunMessageLoop> Coordinate Mapper Changed.\n");
#endif
            break;
#endif
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
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 打开复源帧读取器 需求彩色+玩家索引
    if (SUCCEEDED(hr)){
        hr = m_pKinect->OpenMultiSourceFrameReader(
            FrameSourceTypes::FrameSourceTypes_Color | 
            FrameSourceTypes::FrameSourceTypes_BodyIndex |
            FrameSourceTypes::FrameSourceTypes_Depth,
            &m_pMultiSourceFrameReader
            );
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
#ifdef _DEBUG
        // 检查是否已存在
        if (m_hMultiSourceFrameArrived)
            ::MessageBoxW(m_hwnd, L"成员变量:m_hMultiSourceFrameArrived 值已存在", L"<ThisApp::init_kinect>", MB_ICONERROR);
#endif
        m_pMultiSourceFrameReader->SubscribeMultiSourceFrameArrived(&m_hMultiSourceFrameArrived);
    }
    // 获取坐标映射器
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_CoordinateMapper(&m_pCoordinateMapper);
    }
    // 注册映射器改变事件 因为目前不能改变分辨率 所以还不会改变
    if (SUCCEEDED(hr)){
        hr = m_pCoordinateMapper->SubscribeCoordinateMappingChanged(&m_hCoordinateMapperChanged);
    }
    return hr;
}


// 检查复源帧
void ThisApp::check_color_frame(){
    if (!m_pMultiSourceFrameReader) return;
#ifdef _DEBUG
    static int frame_num = 0;
    ++frame_num; 
    _cwprintf(L"<ThisApp::check_color_frame>Frame@%8d ", frame_num);
#endif 
    // 复源临帧事件参数
    IMultiSourceFrameArrivedEventArgs* pArgs = nullptr;
    // 复源帧引用
    IMultiSourceFrameReference* pMSFrameRef = nullptr;
    // 复源帧
    IMultiSourceFrame* pMultiSourceFrame = nullptr;
    // 彩色帧
    IColorFrame* pColorFrame = nullptr;
    // 帧描述
    IFrameDescription* pColorFrameDescription = nullptr;
    // 彩色帧宽度数据
    int width_color = 0;
    // 彩色帧高度数据
    int height_color = 0;
    // 帧格式
    ColorImageFormat imageFormat = ColorImageFormat_None;
    // 彩色帧缓存大小
    UINT nColorBufferSize = 0;
    // 彩色帧缓存
    RGBQUAD *pColorBuffer = nullptr;

    // 获取参数
    HRESULT hr = m_pMultiSourceFrameReader->GetMultiSourceFrameArrivedEventData(m_hMultiSourceFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pMSFrameRef);
    }
    // 获取复源帧
    if (SUCCEEDED(hr)) {
        hr = pMSFrameRef->AcquireFrame(&pMultiSourceFrame);
    }
    // 获取彩色帧
    if (SUCCEEDED(hr)) {
        IColorFrameReference* pColorFrameReference = nullptr;
        hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
        if (SUCCEEDED(hr)){
            hr = pColorFrameReference->AcquireFrame(&pColorFrame);
        }
        SafeRelease(pColorFrameReference);
    }
    // 获取彩色帧描述
    if (SUCCEEDED(hr)) {
        hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
    }
    // 获取彩色帧宽度
    if (SUCCEEDED(hr)) {
        hr = pColorFrameDescription->get_Width(&width_color);
    }
    // 获取彩色帧高度
    if (SUCCEEDED(hr)) {
        hr = pColorFrameDescription->get_Height(&height_color);
    }
    // 获取帧格式
    if (SUCCEEDED(hr)) {
        hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
    }
    // 获取帧真数据
    if (SUCCEEDED(hr)) {
        // 在已经是BGRA的情况下 获取数据指针
        if (imageFormat == ColorImageFormat_Bgra) {
            hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
        }
        // 负责用自带的方法转换
        else{
            nColorBufferSize = IMAGE_C_WIDTH * IMAGE_C_HEIGHT * sizeof(RGBQUAD);
            pColorBuffer = m_pColorFrameBuffer;
            hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
        }
    }


    // 深度帧
    IDepthFrame* pDepthFrame = nullptr;
    // 深度帧缓存区大小
    UINT32 nDepthFrameBufferSize = 0;
    // 深度帧缓冲区位置
    UINT16* pDepthBuffer = nullptr;
    // 深度帧宽度
    int nDepthFrameWidth = 0;
    // 深度帧高度
    int nDepthFrameHeight = 0;
    // 深度帧描述
    IFrameDescription* pDepthFrameDescription = nullptr;
    // 获取玩家索引帧
    if (SUCCEEDED(hr)) {
        IDepthFrameReference* pDepthFrameReference = nullptr;
        hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
        if (SUCCEEDED(hr)){
            hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
        }
        SafeRelease(pDepthFrameReference);
    }
    // 获取深度帧描述
    if (SUCCEEDED(hr)){
        hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
    }
    // 获取深度帧宽度
    if (SUCCEEDED(hr)){
        hr = pDepthFrameDescription->get_Width(&nDepthFrameWidth);
    }
    // 获取深度帧高度
    if (SUCCEEDED(hr)){
        hr = pDepthFrameDescription->get_Height(&nDepthFrameHeight);
    }
    // 获取索引数据
    if (SUCCEEDED(hr)){
        hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthFrameBufferSize, &pDepthBuffer);
    }


    // 玩家索引帧
    IBodyIndexFrame* pBodyIndexFrame = nullptr;
    // 索引缓存区大小
    UINT32 nBodyIndexSize = 0;
    // 索引缓冲区位置
    BYTE* pIndexBuffer = nullptr;
    // 获取玩家索引帧
    if (SUCCEEDED(hr)) {
        IBodyIndexFrameReference* pBodyIndexFrameReference = nullptr;
        hr = pMultiSourceFrame->get_BodyIndexFrameReference(&pBodyIndexFrameReference);
        if (SUCCEEDED(hr)){
            hr = pBodyIndexFrameReference->AcquireFrame(&pBodyIndexFrame);
        }
        SafeRelease(pBodyIndexFrameReference);
    }
    // 获取索引数据
    if (SUCCEEDED(hr)){
        pBodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexSize, &pIndexBuffer);
    }


    // 进行坐标映射 以彩色帧为基础
    if (SUCCEEDED(hr)){
       // 测试: 获取成功后不再更新坐标
#if 0
       static bool called = true;
       if (called){
           hr = m_pCoordinateMapper->MapDepthFrameToColorSpace(
               nDepthFrameWidth * nDepthFrameHeight, (UINT16*)pDepthBuffer, nDepthFrameWidth * nDepthFrameHeight, m_pColorCoordinates
               );
           if (SUCCEEDED(hr))
               called = false;
       }
#else
        hr = m_pCoordinateMapper->MapDepthFrameToColorSpace(
            nDepthFrameWidth * nDepthFrameHeight, (UINT16*)pDepthBuffer, nDepthFrameWidth * nDepthFrameHeight, m_pColorCoordinates
            );
#endif
    }
    // 处理数据
    if (SUCCEEDED(hr)){
        auto buffer = m_ImagaRenderer.GetBuffer();
        int x, y; RGBQUAD color;
        for (UINT i = 0; i < nDepthFrameBufferSize; ++i){
            ColorSpacePoint colorPoint = m_pColorCoordinates[i];
            x = static_cast<int>(floor(colorPoint.X + .5F));
            y = static_cast<int>(floor(colorPoint.Y + .5F));
            if (pIndexBuffer[i] == 0xFF || x >= width_color || y >= height_color || x < 0 || y < 0){
                color = {0};
            }
            else{
                color = pColorBuffer[y*width_color + x];
            }
            buffer[i] = color;
        }
    }
    // 传输数据
    if (SUCCEEDED(hr)){
        m_ImagaRenderer.WriteBitmapData(nDepthFrameWidth, nDepthFrameHeight);
    }
    // 安全释放
    SafeRelease(pDepthFrameDescription);
    SafeRelease(pDepthFrame);


    SafeRelease(pBodyIndexFrame);

    SafeRelease(pColorFrameDescription);
    SafeRelease(pColorFrame);
    SafeRelease(pMultiSourceFrame);
    SafeRelease(pMSFrameRef);
    SafeRelease(pArgs);
#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _cwprintf(L" 成功\n");
    else
        _cwprintf(L" 失败\n");
#endif
}