#include "stdafx.h"
#include "included.h"



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
    // 释放HighDefinitionFaceFrameReader
    SafeRelease(m_pHDFaceFrameReader);
    // 释放HighDefinitionFaceFrameSource
    SafeRelease(m_pHDFaceFrameSource);
    // 释放ColorFrameReader
    SafeRelease(m_pColorFrameReader);
    // 释放BodyFrameReader
    SafeRelease(m_pBodyFrameReader);
    // 释放FaceAlignment
    SafeRelease(m_pFaceAlignment);
    // 释放Mapper
    SafeRelease(m_pMapper);
    // 释放Face Model
    SafeRelease(m_pFaceModel);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
        m_pKinect->Release();
        m_pKinect = nullptr;
    }
    // 释放缓存
    if (m_pFaceVertices){
        free(m_pFaceVertices);
        m_pFaceVertices = nullptr;
        const_cast<const ColorSpacePoint*>(m_ImagaRenderer.data.face_points) = nullptr;
        const_cast<UINT&>(m_ImagaRenderer.data.face_points_count) = 0;
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
        reinterpret_cast<HANDLE>(m_hColorFrameArrived),
        reinterpret_cast<HANDLE>(m_hBodyFrameArrived),
        reinterpret_cast<HANDLE>(m_hHDFFrameArrived),
        
    };
    while (true){
        // 消息处理
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        // 设置事件
        // 事件0: 彩色临帧事件
        events[0] = reinterpret_cast<HANDLE>(m_hColorFrameArrived);
        // 事件1: 骨骼临帧事件
        events[1] = reinterpret_cast<HANDLE>(m_hBodyFrameArrived);
        // 事件2: 高清面部临帧事件
        events[2] = reinterpret_cast<HANDLE>(m_hHDFFrameArrived);
        // 检查事件
        switch (MsgWaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
            // events[0]
        case WAIT_OBJECT_0 + 0:
            this->check_color_frame();
            break;
            // events[1]
        case WAIT_OBJECT_0 + 1:
            this->check_body_frame();
            break;
            // events[2]
        case WAIT_OBJECT_0 + 2:
            this->check_hd_face_frame();
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
    IBodyFrameSource* pBodyFrameSource = nullptr;
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
        hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        m_pColorFrameReader->SubscribeFrameArrived(&m_hColorFrameArrived);
    }
    // 获取骨骼帧源
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_BodyFrameSource(&pBodyFrameSource);
    }
    // 获取骨骼帧读取器
    if (SUCCEEDED(hr)) {
        hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        m_pBodyFrameReader->SubscribeFrameArrived(&m_hBodyFrameArrived);
    }
    // 创建高清面部帧源
    if (SUCCEEDED(hr)){
        hr = CreateHighDefinitionFaceFrameSource(m_pKinect, &m_pHDFaceFrameSource);
    }
    // 创建高清面部帧读取器
    if (SUCCEEDED(hr)){
        hr = m_pHDFaceFrameSource->OpenReader(&m_pHDFaceFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        hr = m_pHDFaceFrameReader->SubscribeFrameArrived(&m_hHDFFrameArrived);
    }
    // 创建面部特征对齐
    if (SUCCEEDED(hr)){
        hr = CreateFaceAlignment(&m_pFaceAlignment);
    }
    // 创建面部模型
    if (SUCCEEDED(hr)){
        hr = CreateFaceModel(1.f, FaceShapeDeformations::FaceShapeDeformations_Count, m_ImagaRenderer.data.sd, &m_pFaceModel);
    }
    // 获取映射器
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_CoordinateMapper(&m_pMapper);
    }
    // 获取面部定点数
    if (SUCCEEDED(hr)){
        hr = GetFaceModelVertexCount(&m_cFaceVerticeCount);
    }
    // 创建顶点缓存
    if (SUCCEEDED(hr)){
        m_pFaceVertices = reinterpret_cast<CameraSpacePoint*>(malloc(
            (sizeof(CameraSpacePoint) + sizeof(ColorSpacePoint)) * m_cFaceVerticeCount)
            );
        if (!m_pFaceVertices) hr = E_OUTOFMEMORY;
    }
    // 修改数据
    if (SUCCEEDED(hr)){
        const_cast<const ColorSpacePoint*>(m_ImagaRenderer.data.face_points) =
            reinterpret_cast<const ColorSpacePoint*>(m_pFaceVertices + m_cFaceVerticeCount);
        const_cast<UINT&>(m_ImagaRenderer.data.face_points_count) = m_cFaceVerticeCount;
    }
    SafeRelease(pColorFrameSource);
    SafeRelease(pBodyFrameSource);
    return hr;
}


// 检查彩色帧
void ThisApp::check_color_frame(){
    if (!m_pColorFrameReader) return;
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
}



// 检查骨骼帧
void ThisApp::check_body_frame(){
    // 骨骼临帧事件参数
    IBodyFrameArrivedEventArgs* pArgs = nullptr;
    // 骨骼帧引用
    IBodyFrameReference* pBFrameRef = nullptr;
    // 骨骼帧
    IBodyFrame* pBodyFrame = nullptr;
    // 骨骼
    IBody*  ppBody[BODY_COUNT] = { 0 };

    // 获取参数
    HRESULT hr = m_pBodyFrameReader->GetFrameArrivedEventData(m_hBodyFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pBFrameRef);
    }
    // 获取骨骼帧
    if (SUCCEEDED(hr)) {
        hr = pBFrameRef->AcquireFrame(&pBodyFrame);
    }
    // 获取骨骼数据
    if (SUCCEEDED(hr)) {
        hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, ppBody);
    }
    // 高清面部帧源未被跟踪时尝试更换ID
    BOOLEAN tracked = FALSE;
    // 检查是否未被跟踪
    if (SUCCEEDED(hr)){
        hr = m_pHDFaceFrameSource->get_IsTrackingIdValid(&tracked);
    }
    // 高清面部帧源未被跟踪时尝试更换ID
    if (SUCCEEDED(hr) && !tracked){
        for (int i = 0; i < BODY_COUNT; ++i) {
            hr = ppBody[i]->get_IsTracked(&tracked);
            if (SUCCEEDED(hr) && tracked){
                UINT64 id = 0;
                if (FAILED(ppBody[i]->get_TrackingId(&id))) continue;
#ifdef _DEBUG
                _cwprintf(L"更换ID: %08X-%08X\n", HIDWORD(id), LODWORD(id));
#endif
                m_pHDFaceFrameSource->put_TrackingId(id);
                break;
            }
        }

    }
    // 安全释放
    for (int i = 0; i < BODY_COUNT; ++i) SafeRelease(ppBody[i]);
    SafeRelease(pBodyFrame);
    SafeRelease(pBFrameRef);
    SafeRelease(pArgs);
}


// 检查高清面部帧
void ThisApp::check_hd_face_frame(){
    m_ImagaRenderer.data.tracked = FALSE;
#ifdef _DEBUG
    static int frame_num = 0;
    ++frame_num;
    _cwprintf(L"<ThisApp::check_hd_face_frame>Frame@%8d ", frame_num);
#endif 
    // 高清面部临帧事件参数
    IHighDefinitionFaceFrameArrivedEventArgs* pArgs = nullptr;
    // 高清面部帧引用
    IHighDefinitionFaceFrameReference* pHDFFrameRef = nullptr;
    // 高清面部帧
    IHighDefinitionFaceFrame* pHDFaceFrame = nullptr;

    // 获取参数
    HRESULT hr = m_pHDFaceFrameReader->GetFrameArrivedEventData(m_hHDFFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pHDFFrameRef);
    }
    // 获取骨骼帧
    if (SUCCEEDED(hr)) {
        hr = pHDFFrameRef->AcquireFrame(&pHDFaceFrame);
    }
    // 更新面部特征对齐
    if (SUCCEEDED(hr)){
        hr = pHDFaceFrame->GetAndRefreshFaceAlignmentResult(m_pFaceAlignment);
    }
    // 获取面部顶点
    if (SUCCEEDED(hr)){
        hr = m_pFaceModel->CalculateVerticesForAlignment(m_pFaceAlignment, m_cFaceVerticeCount, m_pFaceVertices);
    }
    /*/ 获取Face Shape Deformations
    if (SUCCEEDED(hr)){
       hr = m_pFaceModel->GetFaceShapeDeformations(FaceShapeDeformations_Count, m_ImagaRenderer.data.sd);
    }*/
    // 成功
    if (SUCCEEDED(hr)){
        for (UINT i = 0U; i < m_cFaceVerticeCount; ++i){
            m_pMapper->MapCameraPointsToColorSpace(m_cFaceVerticeCount, m_pFaceVertices,
                m_ImagaRenderer.data.face_points_count,
                const_cast<ColorSpacePoint*>(m_ImagaRenderer.data.face_points)
                );
        }
        m_ImagaRenderer.data.tracked = TRUE;
    }
    // 安全释放
    SafeRelease(pHDFaceFrame);
    SafeRelease(pHDFFrameRef);
    SafeRelease(pArgs);

#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _cwprintf(L" 成功\n");
    else
        _cwprintf(L" 失败\n");
#endif
}