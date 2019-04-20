#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"

//#define RTRACE(a)


// ThisApp构造函数
ThisApp::ThisApp(WCHAR* file_name):m_ImagaRenderer(this){
    // 复制字符串
    wcscpy_s(m_szFileNameBuffer, file_name);
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 释放 Coordinate Mapper
    SafeRelease(m_pCoordinateMapper);
    // 释放 Color Frame Reader
    SafeRelease(m_pColorFrameReader);
    // 释放 Depth Frame Reader
    SafeRelease(m_pDepthFrameReader);
    // 释放骨骼数据
    for (int i = 0; i < BODY_COUNT; ++i){
        SafeRelease(m_apBodies[i]);
    }
    // 释放手势数据
    this->release_gesture_data();
    // 释放 Gesture Frame Reader
    SafeRelease(m_pGestureFrameReader);
    // 释放 Gesture Frame Source
    SafeRelease(m_pGestureFrameSource);
    // 动态申请了内存的话需要释放掉
    if (m_ppGestures != m_apGestures){
        delete[] m_ppGestures;
    }
    m_ppGestures = nullptr;
    m_cGesturesBufferSize = 0;
    // 释放BodyFrameReader
    SafeRelease(m_pBodyFrameReader);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    SafeRelease(m_pKinect);
}

// 载入手势文件
HRESULT ThisApp::load_gesture_database_file(const WCHAR* file_name){
    if (*file_name == 0) return S_FALSE;
    // 复制字符串
    if (file_name != m_szFileNameBuffer)
        wcscpy_s(m_szFileNameBuffer, file_name);
    // 先释放
    this->release_gesture_data();
    // 再载入
    HRESULT hr = S_OK;
    // 载入手势文件
    if (SUCCEEDED(hr)){
        hr = CreateVisualGestureBuilderDatabaseInstanceFromFile(file_name, &m_pGestureDatabase);
    }
    // 获取手势数量
    if (SUCCEEDED(hr)){
        hr = m_pGestureDatabase->get_AvailableGesturesCount(&m_cGestureSize);
    }
    // 缓存不足则重新申请
    if (SUCCEEDED(hr)){
        if (m_cGestureSize > m_cGesturesBufferSize){
            // 扩展为2倍
            m_cGesturesBufferSize = m_cGestureSize * 2;
            m_vGesturesInfo.resize(m_cGesturesBufferSize);
            // 已经申请过了则释放
            if (m_ppGestures != m_apGestures) delete[] m_ppGestures;
            // 申请
            m_ppGestures = new IGesture*[m_cGesturesBufferSize];
            // 检查指针
            if (!m_ppGestures) hr = E_OUTOFMEMORY;
        }
    }
    // 获取手势
    if (SUCCEEDED(hr)){
        hr = m_pGestureDatabase->get_AvailableGestures(m_cGestureSize, m_apGestures);
    }
    // 添加手势
    if (SUCCEEDED(hr)){
        hr = m_pGestureFrameSource->AddGestures(m_cGestureSize, m_apGestures);
    }
    return hr;
}
// 释放手势
void ThisApp::release_gesture_data(){
    // 释放手势数据指针
    for (UINT i = 0U; i < m_cGestureSize; ++i){
        // 去除手势
        m_pGestureFrameSource->RemoveGesture(m_ppGestures[i]);
        // 释放手势
        SafeRelease(m_ppGestures[i]);
    }
    m_cGestureSize = 0;
    // 释放 Gesture Database
    SafeRelease(m_pGestureDatabase);
}

// 初始化
HRESULT ThisApp::Initialize(HINSTANCE hInstance, int nCmdShow){
    HRESULT hr = E_FAIL;
    if (SUCCEEDED(static_cast<HRESULT>(m_ImagaRenderer)))
    {
        //register window class
        WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = ThisApp::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = L"Direct2DTemplate";
        wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));;
        // 注册窗口
        RegisterClassExW(&wcex);
        // 创建窗口
        RECT window_rect = { 0, 0, WNDWIDTH, WNDHEIGHT };
        DWORD window_style = WS_OVERLAPPEDWINDOW;
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
            // 允许拖拽
            DragAcceptFiles(m_hwnd, TRUE);
            // 设置定时器
            SetTimer(m_hwnd, 1, 20, nullptr);
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
        if (FAILED(pOurApp->init_kinect()) || FAILED(pOurApp->load_gesture_database_file(pOurApp->m_szFileNameBuffer))){
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
            case WM_TIMER:
                // 检查
                pOurApp->check_color_frame();
                pOurApp->check_depth_frame();
                pOurApp->check_body_frame();
                pOurApp->check_gesture_frame();
                // 渲染
                pOurApp->m_ImagaRenderer.OnRender();
                break;
            case WM_MOUSEWHEEL:
                pOurApp->m_ImagaRenderer.show_offset += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam) * 0.25f);
                //pOurApp->m_ImagaRenderer.OnRender();
                break;
            case WM_DROPFILES:
                // 拖放文件
            {
                HDROP hDrop = reinterpret_cast<HDROP>(wParam);
                WCHAR buffer[MAX_PATH * 4];
                // 获取第一个
                DragQueryFileW(hDrop, 0, buffer, lengthof(buffer));
                pOurApp->load_gesture_database_file(buffer);
                // 释放空间
                DragFinish(hDrop);
            }
                break;
            case WM_SIZE:
                // 改变窗口大小
                pOurApp->m_ImagaRenderer.OnSize(LOWORD(lParam), HIWORD(lParam));
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
    IDepthFrameSource* pDepthFrameSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // --------------  手势帧部分 --------------
    // 创建手势帧源
    if (SUCCEEDED(hr)){
        hr = CreateVisualGestureBuilderFrameSource(m_pKinect, 0, &m_pGestureFrameSource);
    }
    // 获取读取器
    if (SUCCEEDED(hr)){
        hr = m_pGestureFrameSource->OpenReader(&m_pGestureFrameReader);
    }
    // --------------  彩色帧部分 --------------
    // 获取彩色帧源(ColorFrameSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_ColorFrameSource(&pColorFrameSource);
    }
    // 再获取彩色帧读取器
    if (SUCCEEDED(hr)){
        hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
    }
    // --------------  深度帧部分 --------------
    // 获取深度帧源(DepthFrameSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_DepthFrameSource(&pDepthFrameSource);
    }
    // 再获取彩色帧读取器
    if (SUCCEEDED(hr)){
        hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
    }
    // --------------  骨骼帧部分 --------------
    // 获取骨骼帧源(BodyFrameSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_BodyFrameSource(&pBodyFrameSource);
    }
    // 再获取骨骼帧读取器
    if (SUCCEEDED(hr)){
        hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
    }
    // --------------  坐标映射器部分 --------------
    // 获取坐标映射器
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_CoordinateMapper(&m_pCoordinateMapper);
    }
    // 扫尾工作
    SafeRelease(pColorFrameSource);
    SafeRelease(pDepthFrameSource);
    SafeRelease(pBodyFrameSource);
    
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
    if (!pColorFrame) return;
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
    // 获取帧真数据
    if (SUCCEEDED(hr)) {
        // 在已经是BGRA的情况下 直接获取源数据
        if (imageFormat == ColorImageFormat_Bgra) {
            hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
        }
        // 负责用自带的方法转换
        else{
            pBuffer = m_ImagaRenderer.GetColorBuffer();
            nBufferSize = IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(RGBQUAD);
            hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
        }
    }
    // 传输数据
    if (SUCCEEDED(hr)){
        m_ImagaRenderer.WriteBitmapData(pBuffer, 0, width, height);
    }

    SafeRelease(pColorFrame);
    SafeRelease(pColorFrameDescription);
}

// 检查深度帧
void ThisApp::check_depth_frame(){
    if (!m_pDepthFrameReader) return;
    // 深度帧
    IDepthFrame* pDepthFrame = nullptr;
    // 帧描述
    IFrameDescription* pFrameDescription = nullptr;
    // 深度帧宽度数据
    int width = 0;
    // 深度帧高度数据
    int height = 0;
    // 最近有效值
    USHORT depth_min_reliable_distance = 0;
    // 最远有效值
    USHORT depth_max_reliable_distance = 0;
    // 帧缓存大小
    UINT nBufferSize = 0;
    // 深度缓存
    UINT16 *pBuffer = nullptr;

    // 获取深度帧
    HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
    if (!pDepthFrame) return;
    // 获取帧描述
    if (SUCCEEDED(hr)) {
        hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
    }
    // 获取帧宽度
    if (SUCCEEDED(hr)) {
        hr = pFrameDescription->get_Width(&width);
    }
    // 获取帧高度
    if (SUCCEEDED(hr)) {
        hr = pFrameDescription->get_Height(&height);
    }
    // 获取最近有效距离值
    if (SUCCEEDED(hr)) {
        hr = pDepthFrame->get_DepthMinReliableDistance(&depth_min_reliable_distance);
    }
    // 获取最远有效距离值
    if (SUCCEEDED(hr))  {
        hr = pDepthFrame->get_DepthMaxReliableDistance(&depth_max_reliable_distance);
    }
    // 获取深度数据
    if (SUCCEEDED(hr))  {
        hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
    }
    // 处理数据
    if (SUCCEEDED(hr)) {
        auto pRGBXBuffer = m_ImagaRenderer.GetDepthBuffer();
        // 处理算法
        // 0着红色 (0, min)着128~255渐进绿色 大于max的着128~255蓝色 之间的着0~255灰色
        // 不同深度渐进过度
        for (UINT i = 0; i < nBufferSize; ++i){
            if (!pBuffer[i]){
                pRGBXBuffer[i].rgbRed = 0xFF;
                pRGBXBuffer[i].rgbGreen = 0;
                pRGBXBuffer[i].rgbBlue = 0;
                pRGBXBuffer[i].rgbReserved = 0xFF;
            }
            else if (pBuffer[i] < depth_min_reliable_distance){
                pRGBXBuffer[i].rgbRed = 0;
                pRGBXBuffer[i].rgbGreen = pBuffer[i] & 0x7F + 0x80;
                pRGBXBuffer[i].rgbBlue = 0;
                pRGBXBuffer[i].rgbReserved = 0xFF;
            }
            else if (pBuffer[i] > depth_max_reliable_distance){
                pRGBXBuffer[i].rgbBlue = pBuffer[i] & 0x7F + 0x80;
                pRGBXBuffer[i].rgbGreen = 0;
                pRGBXBuffer[i].rgbRed = 0;
                pRGBXBuffer[i].rgbReserved = 0xFF;
            }
            else{
                pRGBXBuffer[i].rgbBlue = pBuffer[i] & 0xFF;
                pRGBXBuffer[i].rgbGreen = pRGBXBuffer[i].rgbBlue;
                pRGBXBuffer[i].rgbRed = pRGBXBuffer[i].rgbBlue;
                pRGBXBuffer[i].rgbReserved = 0xFF;
            }
        }
        // 传输数据
        m_ImagaRenderer.WriteBitmapData(pRGBXBuffer, 1, width, height);
    }
    // 安全释放
    SafeRelease(pFrameDescription);
    SafeRelease(pDepthFrame);
}

// 检查手势帧
void ThisApp::check_gesture_frame(){
    // 获取最近一帧手势
    IVisualGestureBuilderFrame* pGestureFrame = nullptr;
    if (SUCCEEDED(m_pGestureFrameReader->CalculateAndAcquireLatestFrame(&pGestureFrame))){
        // 检查连续型手势
        IContinuousGestureResult* pCGResult = nullptr;
        // 检查离散性手势
        IDiscreteGestureResult* pDGResult = nullptr;
        // 循环检查
        for (UINT i = 0U; i < m_cGestureSize; ++i){
            // 获取信息
            auto info_data = &(m_vGesturesInfo[i]);
            // 推进索引
            if (info_data->index >= GESTURES_SAVED){
                info_data->index = 0;
            }
            else{
                ++info_data->index;
            }
            // 获取连续型手势结果
            pGestureFrame->get_ContinuousGestureResult(m_apGestures[i], &pCGResult);
            if (pCGResult){
                info_data->type = 0;
                pCGResult->get_Progress(info_data->float_var + info_data->index);
                // 释放对象
                pCGResult->Release();
                pCGResult = nullptr;
                continue;
            }
            // 获取离散性手势结果
            pGestureFrame->get_DiscreteGestureResult(m_apGestures[i], &pDGResult);
            if (pDGResult){
                info_data->type = 1;
                // 获取置信值
                pDGResult->get_Confidence(info_data->float_var + info_data->index);
                // 获取检查状态
                pDGResult->get_Detected(info_data->bool1_var + info_data->index);
                // 获取首帧状态
                pDGResult->get_FirstFrameDetected(info_data->bool2_var + info_data->index);
                // 释放对象
                pDGResult->Release();
                pDGResult = nullptr;
            }
        }
    }
    SafeRelease(pGestureFrame);
}

// 检查骨骼帧
void ThisApp::check_body_frame(){
    IBodyFrame* pBodyFrame = nullptr;

    // 获取骨骼帧
    HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
    if (!pBodyFrame) return;
    // 获取骨骼数据
    if (SUCCEEDED(hr)) {
        hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, m_apBodies);
    }
    // 处理数据
    if (SUCCEEDED(hr)){
        for (int i = 0; i < BODY_COUNT; ++i){
            // 保证数据
            BOOLEAN bTracked = false;
            hr = m_apBodies[i]->get_IsTracked(&bTracked);
            if (!m_apBodies[i] || FAILED(hr) || !bTracked) continue;
            IBody* pNowBody = m_apBodies[i];
            // 声明数据
            BodyInfo info;
            // 获取左右手状态
            pNowBody->get_HandLeftState(&info.leftHandState);
            pNowBody->get_HandRightState(&info.rightHandState);
            // 获取骨骼位置信息
            hr = pNowBody->GetJoints(JointType_Count, info.joints);
            if (SUCCEEDED(hr)) {
                // 坐标转换
                for (int j = 0; j < JointType_Count; ++j){
                    m_pCoordinateMapper->MapCameraPointToDepthSpace(
                        info.joints[j].Position,
                        reinterpret_cast<DepthSpacePoint*>(info.jointPoints + j)
                        );
                }
                // 设置数据
                m_ImagaRenderer.SetBodyInfo(i, &info);
            }
        }
    }
    // 安全释放
    SafeRelease(pBodyFrame);
    // 在手势帧没有跟踪时尝试更换id
    BOOLEAN tracked = TRUE;
    m_pGestureFrameSource->get_IsTrackingIdValid(&tracked);
    if (tracked) return;
    // 更换
    UINT64 id = 0;
    for (int i = 0; i < BODY_COUNT; ++i){
        m_apBodies[i]->get_IsTracked(&tracked);
        if (tracked){
            m_apBodies[i]->get_TrackingId(&id);
            m_pGestureFrameSource->put_TrackingId(id);
#ifdef _DEBUG
            _cwprintf(L"手势: 更换ID: %l64d\n", id);
#endif
            break;
        }
    }
}

