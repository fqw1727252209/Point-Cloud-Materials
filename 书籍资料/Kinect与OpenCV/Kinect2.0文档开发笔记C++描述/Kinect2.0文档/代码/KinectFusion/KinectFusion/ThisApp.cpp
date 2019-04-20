#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) sizeof(a)/sizeof(*a)


void SetIdentityMatrix(Matrix4 &mat)
{
    mat.M11 = 1.f; mat.M12 = 0.f; mat.M13 = 0.f; mat.M14 = 0.f;
    mat.M21 = 0.f; mat.M22 = 1.f; mat.M23 = 0.f; mat.M24 = 0.f;
    mat.M31 = 0.f; mat.M32 = 0.f; mat.M33 = 1.f; mat.M34 = 0.f;
    mat.M41 = 0.f; mat.M42 = 0.f; mat.M43 = 0.f; mat.M44 = 1.f;
}

// 重置
void ThisApp::ResetReconstruction(){
    m_pReconstruction->ResetReconstruction(&m_worldToCameraTransform, &m_defaultWorldToVolumeTransform);
    //m_pReconstruction->ResetReconstruction(nullptr, nullptr);
}

// ThisApp构造函数
ThisApp::ThisApp(){
    
    // 重建参数
    m_reconstructionParams.voxelsPerMeter = 256.f;
    m_reconstructionParams.voxelCountX = 384;
    m_reconstructionParams.voxelCountY = 384; 
    m_reconstructionParams.voxelCountZ = 384; 
    // 使用AMP进行运算
    m_processorType = NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_AMP;

    // 设置为单位矩阵
    SetIdentityMatrix(m_worldToCameraTransform);
    SetIdentityMatrix(m_defaultWorldToVolumeTransform);
    // 
    m_cameraParameters.focalLengthX = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_X;
    m_cameraParameters.focalLengthY = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_Y;
    m_cameraParameters.principalPointX = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_X;
    m_cameraParameters.principalPointY = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_Y;
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 销毁事件
    if (m_hDepthFrameArrived && m_pDepthFrameReader){
        m_pDepthFrameReader->UnsubscribeFrameArrived(m_hDepthFrameArrived);
        m_hDepthFrameArrived = 0;
    }
    // 释放DepthFrameReader
    SafeRelease(m_pDepthFrameReader);
    // 清理Fusion数据
    SafeRelease(m_pReconstruction);
    SafeRelease(m_pMapper);
    // 清理Fusion图像帧
    SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pSurfaceImageFrame);
    SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pPointCloud);
    SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pDepthFloatImage);
    SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pSmoothDepthFloatImage);
    SAFE_FUSION_RELEASE_IMAGE_FRAME(m_pNormalImageFrame);
    // 清理缓存
    SAFE_DELETE_ARRAY(m_pDepthImagePixelBuffer);
    SAFE_DELETE_ARRAY(m_pDepthDistortionMap);
    SAFE_DELETE_ARRAY(m_pDepthDistortionLT);
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
        reinterpret_cast<HANDLE>(m_hDepthFrameArrived),
        reinterpret_cast<HANDLE>(m_coordinateMappingChangedEvent),
    };
    while (true){
        // 消息处理
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        // 设置事件
        // 事件0: 深度临帧事件
        events[0] = reinterpret_cast<HANDLE>(m_hDepthFrameArrived);
        // 事件1: 坐标映射改变事件
        events[1] = reinterpret_cast<HANDLE>(m_coordinateMappingChangedEvent);
        // 检查事件
        switch (MsgWaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
            // events[0]
        case WAIT_OBJECT_0 + 0:
            this->check_depth_frame();
            break;
            // events[1]
        case WAIT_OBJECT_0 + 1:
        {
            int break_point = 9;
        }
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
            case WM_LBUTTONUP:
                // 重置
                pOurApp->ResetReconstruction();
                break;
            case WM_PAINT:
                pOurApp->m_ImagaRenderer.OnRender();
                break;
            case WM_SIZE:
                // 改变窗口大小
                pOurApp->m_ImagaRenderer.OnSize(LOWORD(lParam), HIWORD(lParam));
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
    IDepthFrameSource* pDepthFrameSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 获取深度帧源(DepthFrameSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_DepthFrameSource(&pDepthFrameSource);
    }
    // 再获取深度帧读取器
    if (SUCCEEDED(hr)){
        hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        hr = m_pDepthFrameReader->SubscribeFrameArrived(&m_hDepthFrameArrived);
    }
    // 获取坐标映射器
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_CoordinateMapper(&m_pMapper);
    }
    // 注册映射改变事件
    if (SUCCEEDED(hr)) {
        hr = m_pMapper->SubscribeCoordinateMappingChanged(&m_coordinateMappingChangedEvent);
    }
#if 1
    // 获取设备信息:: 可以用于debug
    WCHAR description[MAX_PATH];
    WCHAR instancePath[MAX_PATH];
    UINT memorySize = 0;
    if (SUCCEEDED(hr)){
        hr = NuiFusionGetDeviceInfo(
            m_processorType,
            m_deviceIndex,
            description,
            lengthof(description),
            instancePath,
            lengthof(instancePath),
            &memorySize
            );
        if (hr == E_NUI_BADINDEX){
            ::MessageBoxW(nullptr, L"需要DX11支持", L"错误", MB_ICONERROR);
        }
    }
#endif
    // 创建Fusion容积重建
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateReconstruction(
            &m_reconstructionParams, 
            m_processorType, 
            m_deviceIndex, 
            &m_worldToCameraTransform,
            &m_pReconstruction
            );
        if (hr == E_NUI_GPU_FAIL){
            ::MessageBoxW(nullptr, L"显卡不支持Fusion计算！\n或者 初始化失败", L"错误", MB_ICONERROR);
        }
        else if (hr == E_NUI_GPU_OUTOFMEMORY){
            ::MessageBoxW(nullptr, L"显存不足", L"错误", MB_ICONERROR);
        }
    }
    // 先获取当前的世界到容积转变矩阵并保存 方便以后使用
    if (SUCCEEDED(hr)){
        hr = m_pReconstruction->GetCurrentWorldToVolumeTransform(&m_defaultWorldToVolumeTransform);
    }
    // 创建浮点深度帧
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateImageFrame(
            NUI_FUSION_IMAGE_TYPE_FLOAT, 
            m_cDepthWidth, 
            m_cDepthHeight, 
            nullptr,
            &m_pDepthFloatImage
            );
    }
    // 创建平滑浮点深度帧
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateImageFrame(
            NUI_FUSION_IMAGE_TYPE_FLOAT, 
            m_cDepthWidth, 
            m_cDepthHeight,
            nullptr,
            &m_pSmoothDepthFloatImage
            );
    }
    // 创建点云帧
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateImageFrame(
            NUI_FUSION_IMAGE_TYPE_POINT_CLOUD,
            m_cDepthWidth,
            m_cDepthHeight,
            nullptr,
            &m_pPointCloud
            );
    }
    // 创建Fusion图像帧
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateImageFrame(
            NUI_FUSION_IMAGE_TYPE_COLOR,
            m_cDepthWidth,
            m_cDepthHeight,
            nullptr,
            &m_pSurfaceImageFrame
            );
    }
    // 创建Fusion法线帧
    if (SUCCEEDED(hr)){
        hr = NuiFusionCreateImageFrame(
            NUI_FUSION_IMAGE_TYPE_COLOR,
            m_cDepthWidth,
            m_cDepthHeight,
            nullptr,
            &m_pNormalImageFrame
            );
    }
    SafeRelease(pDepthFrameSource);
    // 重置
    this->ResetReconstruction();
    return hr;
}


// 检查深度帧
void ThisApp::check_depth_frame(){
    if (!m_pDepthFrameReader) return;
#ifdef _DEBUG
    static int frame_num = 0;
    ++frame_num;
    _cwprintf(L"<ThisApp::check_depth_frame>Frame@%8d ", frame_num);
#endif 
    // 深度临帧事件参数
    IDepthFrameArrivedEventArgs* pArgs = nullptr;
    // 深度帧引用
    IDepthFrameReference* pDFrameRef = nullptr;
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

    // 获取参数
    HRESULT hr = m_pDepthFrameReader->GetFrameArrivedEventData(m_hDepthFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pDFrameRef);
    }
    // 获取深度帧
    if (SUCCEEDED(hr)) {
        hr = pDFrameRef->AcquireFrame(&pDepthFrame);
    }
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
    // ----------- Fusion 处理开始
    HRESULT hr4f = hr;
    m_timer.RefreshFrequency();
    m_timer.Start();
    float time__DepthToDepthFloatFrame = 0.0f;
    float time__SmoothDepthFloatFrame = 0.0f;
    float time__ProcessFrame = 0.0f;
    float time__CalculatePointCloud = 0.0f;
    float time__NuiFusionShadePointCloud = 0.0f;
    // 由原深度数据构造浮点数据
    if (SUCCEEDED(hr4f)){
        hr4f = m_pReconstruction->DepthToDepthFloatFrame(
            pBuffer,
            nBufferSize * sizeof(UINT16),
            m_pDepthFloatImage, 
            m_fMinDepthThreshold,
            m_fMaxDepthThreshold,
            true
            );
        time__DepthToDepthFloatFrame = m_timer.DeltaF_ms();
        m_timer.MovStartEnd();
    }
    // 平滑数据
    if (SUCCEEDED(hr4f)){
        hr4f = m_pReconstruction->SmoothDepthFloatFrame(
            m_pDepthFloatImage,
            m_pSmoothDepthFloatImage,
            1,
            0.03f
            );
        time__SmoothDepthFloatFrame = m_timer.DeltaF_ms();
        m_timer.MovStartEnd();
    }
    // 处理当前帧
    if (SUCCEEDED(hr4f)){
        hr4f = m_pReconstruction->ProcessFrame(
            m_pSmoothDepthFloatImage,
            NUI_FUSION_DEFAULT_ALIGN_ITERATION_COUNT,
            m_cMaxIntegrationWeight,
            nullptr,
            &m_worldToCameraTransform
            );
        time__ProcessFrame = m_timer.DeltaF_ms();
        m_timer.MovStartEnd();
    }
    // 检查错误
    if (hr4f == E_NUI_FUSION_TRACKING_ERROR){
        m_ImagaRenderer.error_info = L"Fusion跟踪失败, 请保证目标是静态的,\n请常试重建(单击窗口)";
    }
    else if(SUCCEEDED(hr)){
        m_ImagaRenderer.error_info = L"Fusion跟踪正常";
    }
    else{
        m_ImagaRenderer.error_info = L"Fusion跟踪失败";
    }
    // 获取当前矩阵
    if (SUCCEEDED(hr4f)){
        Matrix4 calculatedCameraPose;
        hr4f = m_pReconstruction->GetCurrentWorldToCameraTransform(&calculatedCameraPose);
        if (SUCCEEDED(hr4f)){
            m_worldToCameraTransform = calculatedCameraPose;
        }
    }
    // 计算点云
    if (SUCCEEDED(hr4f)){
        hr4f = m_pReconstruction->CalculatePointCloud(
            m_pPointCloud, 
            &m_worldToCameraTransform
            );
        time__CalculatePointCloud = m_timer.DeltaF_ms();
        m_timer.MovStartEnd();
    }
    // 生成图像帧
    if (SUCCEEDED(hr4f)){
        // Shading Point Clouid
        Matrix4 worldToBGRTransform = { 0.0f };
        worldToBGRTransform.M11 = m_reconstructionParams.voxelsPerMeter / m_reconstructionParams.voxelCountX;
        worldToBGRTransform.M22 = m_reconstructionParams.voxelsPerMeter / m_reconstructionParams.voxelCountY;
        worldToBGRTransform.M33 = m_reconstructionParams.voxelsPerMeter / m_reconstructionParams.voxelCountZ;
        worldToBGRTransform.M41 = 0.5f;
        worldToBGRTransform.M42 = 0.5f;
        worldToBGRTransform.M43 = 0.0f;
        worldToBGRTransform.M44 = 1.0f;

        //SetIdentityMatrix(worldToBGRTransform);
        //
        hr = NuiFusionShadePointCloud(
            m_pPointCloud, 
            &m_worldToCameraTransform,
            &worldToBGRTransform, 
            m_pSurfaceImageFrame,
            m_pNormalImageFrame
            );
        time__NuiFusionShadePointCloud = m_timer.DeltaF_ms();
        m_timer.MovStartEnd();
    }
    // 数据
    swprintf_s(
        m_ImagaRenderer.profiler_info,
        L"  DepthToDepthFloatFrame:   %6.4fms\n"
        L"  SmoothDepthFloatFrame:    %6.4fms\n"
        L"  ProcessFrame:             %6.4fms\n"
        L"  CalculatePointCloud:      %6.4fms\n"
        L"  NuiFusionShadePointCloud: %6.4fms",
        time__DepthToDepthFloatFrame,
        time__SmoothDepthFloatFrame,
        time__ProcessFrame,
        time__CalculatePointCloud,
        time__NuiFusionShadePointCloud
        );
    // 可视化Fusion数据
    if (SUCCEEDED(hr4f)){

        // Fusion
        m_ImagaRenderer.WriteBitmapData(
            EnumBitmapIndex::Index_Surface,
            reinterpret_cast<RGBQUAD*>(m_pSurfaceImageFrame->pFrameBuffer->pBits),
            width,
            height
            );
        // Normal
        m_ImagaRenderer.WriteBitmapData(
            EnumBitmapIndex::Index_Normal,
            reinterpret_cast<RGBQUAD*>(m_pNormalImageFrame->pFrameBuffer->pBits),
            width,
            height
            );
    }
    // 仅仅是Fusion处理出错的话
    else if (SUCCEEDED(hr)){

    }
    // ----------- Fusion 处理结束
    // 处理数据
    if (SUCCEEDED(hr)) {
        auto pRGBXBuffer = m_ImagaRenderer.GetBuffer();
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
        m_ImagaRenderer.WriteBitmapData(EnumBitmapIndex::Index_Depth, pRGBXBuffer, width, height);
    }
    // 安全释放
    SafeRelease(pFrameDescription);
    SafeRelease(pDepthFrame);
    SafeRelease(pDFrameRef);
    SafeRelease(pArgs);
#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _cwprintf(L" 成功\n");
    else
        _cwprintf(L" 失败\n");
#endif
}