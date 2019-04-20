#include "stdafx.h"
#include "included.h"

//#define TITLE L"Title"


// ThisApp构造函数
ThisApp::ThisApp(){

}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 释放 Depth Frame Reader
    SafeRelease(m_pDepthFrameReader);
    // 释放 Mapper
    SafeRelease(m_pMapper);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
        m_pKinect->Release();
        m_pKinect = nullptr;
    }
}

// 渲染窗口
void ThisApp::Render(ThisApp* pThis){
    // 渲染
    while (true){
        pThis->Update();
        pThis->m_SceneRenderer.OnRender();
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
    wcex.lpszClassName = L"D3D11 Template";
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
        hr = m_SceneRenderer.SetHwnd(m_hwnd);
    }
#ifdef USING_DIRECTINPUT
    // DInput...
    if (SUCCEEDED(hr)){
        hr = KMInput.Init(hInstance, m_hwnd);
    }
#endif
    // 初始化游戏对象
    if (SUCCEEDED(hr)){
        this->initialize_game_objects();
    }
    // 显示窗口
    if (SUCCEEDED(hr)) {
        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);
        m_threadRender.std::thread::~thread();
        m_threadRender.std::thread::thread(Render, this);
    }
#ifdef USING_GAMEIME
    // IME...
    if (SUCCEEDED(hr)){
        m_gameIME.SetHwnd(m_hwnd);
    }
#endif
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
#ifdef USING_GAMEIME
    uint32_t temp_input;
#endif

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        ThisApp *pOurApp = (ThisApp *)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            PtrToUlong(pOurApp)
            );

        if (SUCCEEDED(pOurApp->init_kinect())){
            result = 1;
        }
        else{
            result = 0;
        }
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
#ifdef USING_GAMEIME
            uint32_t flags = pOurApp->m_imeFlag;
#endif
            switch (message)
            {
#ifdef USING_GAMEIME
            case WM_INPUTLANGCHANGE:
                // 输入法改变
                flags |= GameIME_LangChange;
                break;
            case WM_IME_STARTCOMPOSITION:
                pOurApp->m_cInputCount = 0;
                // 短句输入开始
                flags |= GameIME_StartComposition;
                break;
            case WM_IME_ENDCOMPOSITION:
                // 短句输入完成
                flags |= GameIME_EndComposition;
                break;
            case WM_IME_NOTIFY:
                // 不交给上层处理
                result = 1;
                wasHandled = true;
                flags |= 1 << wParam;
                break;
            case WM_CHAR:
                break;
            case WM_IME_CHAR:
                // 上锁
                temp_input = pOurApp->m_cInputCount;
                pOurApp->m_mux4Input.lock();
                pOurApp->m_scBuffer4Input[temp_input] = static_cast<WCHAR>(wParam);
                ++temp_input;
                pOurApp->m_scBuffer4Input[temp_input] = 0;
                // 解锁
                pOurApp->m_mux4Input.unlock();
                pOurApp->m_cInputCount = temp_input;
                break;
#endif
            case WM_LBUTTONDOWN:
                // 记录XY位置
                pOurApp->m_lastFrameX = LOWORD(lParam);
                pOurApp->m_lastFrameY = HIWORD(lParam);
                break;
            case WM_MOUSEWHEEL:
                if (GET_WHEEL_DELTA_WPARAM(wParam) > 0){
                    pOurApp->m_SceneRenderer.z = pOurApp->m_SceneRenderer.z * 0.9f;
                }
                else{
                    pOurApp->m_SceneRenderer.z = pOurApp->m_SceneRenderer.z * 1.1f;
                }
                break;
            case WM_MOUSEMOVE:
                // 左键按下
                if (wParam & MK_LBUTTON){
                    int tx = LOWORD(lParam);
                    int ty = HIWORD(lParam);
                    // 修改
                    pOurApp->m_SceneRenderer.x = pOurApp->m_SceneRenderer.x + static_cast<float>(tx - pOurApp->m_lastFrameX) / 256.f;
                    pOurApp->m_SceneRenderer.y = pOurApp->m_SceneRenderer.y + static_cast<float>(ty - pOurApp->m_lastFrameY) / 256.f;
                    //
                    pOurApp->m_lastFrameX = tx;
                    pOurApp->m_lastFrameY = ty;
                }
                break;
            case WM_CLOSE:
                // 将收尾操作(如结束全部子线程)放在这里
                pOurApp->m_bExit = TRUE;
                // join
                pOurApp->m_threadRender.join();
                // 反初始化
                pOurApp->finalize_game_objects();
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
#ifdef USING_GAMEIME
            pOurApp->m_imeFlag = flags;
#endif
        }
        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}


// 刷新对象
void ThisApp::Update(){
    check_depth_frame();
}


// 初始化游戏对象
void ThisApp::initialize_game_objects(){
    /*ID3D11Device* device = m_SceneRenderer.Get3DDevice();
    
    SafeRelease(device);*/
}

// 反初始化游戏对象
void ThisApp::finalize_game_objects(){


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
    // 获取彩色帧帧形容
    if(SUCCEEDED(hr)){
            float angle_y=45.f;
            // 计算透视转换
            auto projection = DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XM_PI * angle_y / 180.f,
                static_cast<float>(WNDWIDTH) / (static_cast<float>(WNDHEIGHT)),
                SCREEN_NEAR_Z,
                SCREEN_FAR_Z
                );
            // 透视
            DirectX::XMStoreFloat4x4(
                &m_SceneRenderer.scene_model.vscb.projection,
                projection
                );
        
    }
    // 获取映射器
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_CoordinateMapper(&m_pMapper);
    }
    SafeRelease(pDepthFrameSource);
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
    // 获取最新一帧
    HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
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
    // 映射为相机空间坐标
    if (SUCCEEDED(hr)) {
        hr = m_pMapper->MapDepthFrameToCameraSpace(
            DEPTH_WIDTH*DEPTH_HEIGHT,
            pBuffer,
            m_SceneRenderer.scene_model.GetVertexCount(),
            m_SceneRenderer.scene_model.GetVB()
            );
    }
    // 更新数据
    if (SUCCEEDED(hr)){
        m_SceneRenderer.RefreshData();
    }
    // 安全释放
    SafeRelease(pFrameDescription);
    SafeRelease(pDepthFrame);
#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _cwprintf(L"      成功\n");
    else
        _cwprintf(L" 失败\n");
#endif
}