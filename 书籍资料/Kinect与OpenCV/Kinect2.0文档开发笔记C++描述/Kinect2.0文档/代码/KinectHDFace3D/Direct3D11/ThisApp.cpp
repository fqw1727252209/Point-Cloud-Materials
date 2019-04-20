#include "stdafx.h"
#include "included.h"

//#define TITLE L"Title"


static const char* header = R"(
#
# Created via Kinect v2 by dustpg. 2014
# Contact dustpg@gmail.com
#

g face_model



)";

// ThisApp构造函数
ThisApp::ThisApp(){

}

// ThisApp析构函数
ThisApp::~ThisApp(){
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
    // 释放Face Model Builder
    SafeRelease(m_pFaceModelBuilder);
    // 释放 骨骼
    for (int i = 0; i < BODY_COUNT; ++i){
        SafeRelease(m_apBody[i]);
    }
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
        m_pKinect->Release();
        m_pKinect = nullptr;
    }
    // 释放缓存
    if (m_pFaceVertices){
        delete[] m_pFaceVertices;
        m_pFaceVertices = nullptr;
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
            case WM_KEYDOWN:
                if (wParam == 'S'){
                    // 保存数据
                    FILE* file = nullptr;
                    size_t now_length = 0;
                    if (!_wfopen_s(&file, L"FaceModel.obj", L"w")){
                        // 上锁
                        pOurApp->m_muxFaceVertice.lock();
                        // 写开头
                        fprintf(file, header);
                        // 填写顶点坐标
                        for (UINT i = 0u; i < pOurApp->m_cFaceVerticeCount; ++i){
                            fprintf(file, 
                                "v %f %f %f\n",
                                pOurApp->m_pFaceVertices[i].X,
                                pOurApp->m_pFaceVertices[i].Y,
                                pOurApp->m_pFaceVertices[i].Z
                                );
                        }
                        // 解锁
                        pOurApp->m_muxFaceVertice.unlock();
                        // 填写索引信息
                        fprintf(file, "\n\n\n\n");
                        for (UINT32 i = 0u; i < pOurApp->m_SceneRenderer.face_model.index_count; i += 3){
                            fprintf(file,
                                "f %d %d %d\n",
                                pOurApp->m_SceneRenderer.face_model.index[i + 0] + 1,
                                pOurApp->m_SceneRenderer.face_model.index[i + 1] + 1,
                                pOurApp->m_SceneRenderer.face_model.index[i + 2] + 1
                                );
                        }
                    }
                    // 关闭程序
                    if (file){
                        fclose(file);
                        file = nullptr;
                        ::SendMessageW(hwnd, WM_CLOSE, 0, 0);
                    }
                    else{
                        ::MessageBoxW(hwnd, L"储存失败", L"Error", MB_ICONERROR);
                    }
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

    check_color_frame();
    check_body_frame();
    check_hd_face_frame();

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

    IBodyFrameSource* pBodyFrameSource = nullptr;
    IColorFrameSource* pColorFrameSource = nullptr;
    IFrameDescription* pColorFrameDescription = nullptr;
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
    // 获取彩色帧帧形容
    if(SUCCEEDED(hr)){
        hr = pColorFrameSource->get_FrameDescription(&pColorFrameDescription);
        // 设置D3D 相机视野 与Kinect 彩色摄像机一致
        if (pColorFrameDescription){
            int width = 0, height = 0;
            float angle_y=0.f;
            pColorFrameDescription->get_Height(&height);
            pColorFrameDescription->get_Width(&width);
            pColorFrameDescription->get_VerticalFieldOfView(&angle_y);
            // 计算透视转换
            auto projection = DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XM_PI * angle_y / 180.f,
                static_cast<float>(WNDWIDTH) / (static_cast<float>(WNDHEIGHT)),
                SCREEN_NEAR_Z,
                SCREEN_FAR_Z
                );
            // 透视
            DirectX::XMStoreFloat4x4(
                &m_SceneRenderer.face_model.vscb.projection,
                projection
                );
        }
    }
    // 获取骨骼帧源
    if (SUCCEEDED(hr)) {
        hr = m_pKinect->get_BodyFrameSource(&pBodyFrameSource);
    }
    // 获取骨骼帧读取器
    if (SUCCEEDED(hr)) {
        hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
    }
    // 创建高清面部帧源
    if (SUCCEEDED(hr)){
        hr = CreateHighDefinitionFaceFrameSource(m_pKinect, &m_pHDFaceFrameSource);
    }
    // 创建面部模型构建器 之前常试从文件中读取数据
    if (SUCCEEDED(hr)){
        hr = m_pHDFaceFrameSource->OpenModelBuilder(FaceModelBuilderAttributes_None, &m_pFaceModelBuilder);
    }
    // 开始数据收集
    if (SUCCEEDED(hr)){
        hr = m_pFaceModelBuilder->BeginFaceDataCollection();
    }
    // 创建高清面部帧读取器
    if (SUCCEEDED(hr)){
        hr = m_pHDFaceFrameSource->OpenReader(&m_pHDFaceFrameReader);
    }
    // 创建面部特征对齐
    if (SUCCEEDED(hr)){
        hr = CreateFaceAlignment(&m_pFaceAlignment);
    }
    // 创建面部模型
    if (SUCCEEDED(hr)){
        hr = CreateFaceModel(1.f, FaceShapeDeformations::FaceShapeDeformations_Count, m_afFSD, &m_pFaceModel);
    }
    // 获取映射器
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_CoordinateMapper(&m_pMapper);
    }
    // 获取面部顶点数
    if (SUCCEEDED(hr)){
        hr = GetFaceModelVertexCount(&m_cFaceVerticeCount);
    }
    // 创建顶点缓存 与索引缓存
    if (SUCCEEDED(hr)){
        UINT32 index_size = 0;
        // 获取三角形数量
        GetFaceModelTriangleCount(&index_size);
        // 计算索引数量(x 3)
        index_size *= 3;
        m_SceneRenderer.face_model.index_count = index_size;
        // 一次性分配3个缓存
        m_pFaceVertices = reinterpret_cast<CameraSpacePoint*>(malloc(
            (sizeof(CameraSpacePoint) + sizeof(SceneRenderer::VertexNormal))* m_cFaceVerticeCount +
            sizeof(UINT32) * index_size
            ));
        if (!m_pFaceVertices) hr = E_OUTOFMEMORY;
    }
    // 
    if (SUCCEEDED(hr)){
        // 分解1: 计算模型顶点缓存地址
        m_SceneRenderer.face_model.vertex_count = m_cFaceVerticeCount;
        m_SceneRenderer.face_model.face_mash_vertexs = reinterpret_cast<SceneRenderer::VertexNormal*>(
            m_pFaceVertices + m_cFaceVerticeCount
            );
        // 分解2: 计算模型索引缓存地址
        m_SceneRenderer.face_model.index = reinterpret_cast<UINT32*>(
            m_SceneRenderer.face_model.face_mash_vertexs + m_cFaceVerticeCount
            );
        // 获取索引
        GetFaceModelTriangles(
            m_SceneRenderer.face_model.index_count,
            m_SceneRenderer.face_model.index
            );
        ZeroMemory(
            m_SceneRenderer.face_model.face_mash_vertexs, 
            sizeof(SceneRenderer::VertexNormal)* m_cFaceVerticeCount
            );
    }
    SafeRelease(pColorFrameDescription);
    SafeRelease(pColorFrameSource);
    SafeRelease(pBodyFrameSource);
    return hr;
}


// 检查彩色帧
void ThisApp::check_color_frame(){
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
    // 获取彩色帧
    HRESULT  hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
    if (!pColorFrame) {
        return;
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
        // 否则用自带的方法转换
        else{
            pBuffer = m_SceneRenderer.GetBuffer();
            nBufferSize = IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(RGBQUAD);
            hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
        }
    }
    // 传输数据
    if (SUCCEEDED(hr)){
        m_SceneRenderer.WriteBitmapData(pBuffer, width, height);
    }
    // 安全释放
    SafeRelease(pFrameDescription);
    SafeRelease(pColorFrame);
}



// 检查骨骼帧
void ThisApp::check_body_frame(){
    // 骨骼帧
    IBodyFrame* pBodyFrame = nullptr;
    HRESULT hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
    if (!pBodyFrame) return;
    // 获取骨骼数据
    if (SUCCEEDED(hr)) {
        hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, m_apBody);
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
            hr = m_apBody[i]->get_IsTracked(&tracked);
            if (SUCCEEDED(hr) && tracked){
                UINT64 id = 0;
                if (FAILED(m_apBody[i]->get_TrackingId(&id))) continue;
#ifdef _DEBUG
                _cwprintf(L"更换ID: %08X-%08X\n", HIDWORD(id), LODWORD(id));
#endif
                m_pHDFaceFrameSource->put_TrackingId(id);
                break;
            }
        }

    }
    SafeRelease(pBodyFrame);
}


// 检查高清面部帧
void ThisApp::check_hd_face_frame(){
    // 高清面部帧
    IHighDefinitionFaceFrame* pHDFaceFrame = nullptr;

    // 获取骨骼帧
    HRESULT hr = m_pHDFaceFrameReader->AcquireLatestFrame(&pHDFaceFrame);
    if (!pHDFaceFrame) 
        return;
    // 更新面部特征对齐
    if (SUCCEEDED(hr)){
        hr = pHDFaceFrame->GetAndRefreshFaceAlignmentResult(m_pFaceAlignment);
    }
    // 检查面部模型构建器
    if (SUCCEEDED(hr) && !m_bProduced){
        IFaceModelData* pFaceModelData = nullptr;
        // 检查收集状态
        hr = m_pFaceModelBuilder->get_CollectionStatus(&m_SceneRenderer.face_model.co_status);
        // 检查采集状态
        if (SUCCEEDED(hr)){
            hr = m_pFaceModelBuilder->get_CaptureStatus(&m_SceneRenderer.face_model.ca_status);
        }
        // 采集成功 获取数据
        if (SUCCEEDED(hr) && m_SceneRenderer.face_model.co_status == FaceModelBuilderCollectionStatus_Complete){
            hr = m_pFaceModelBuilder->GetFaceData(&pFaceModelData);
        }
        // 生成面部模型
        if (SUCCEEDED(hr) && pFaceModelData){
            SafeRelease(m_pFaceModel);
            hr = pFaceModelData->ProduceFaceModel(&m_pFaceModel);
        }
        // 检查结果
        if (SUCCEEDED(hr) && pFaceModelData){
            m_bProduced = TRUE;
            // 顺便输出数据
            m_pFaceModel->GetFaceShapeDeformations(lengthof(m_afFSD), m_afFSD);
        }
        // 释放掉
        SafeRelease(pFaceModelData);
    }
    // 获取面部顶点
    if (SUCCEEDED(hr)){
        // 上锁
        m_muxFaceVertice.lock();
        // 获取数据
        hr = m_pFaceModel->CalculateVerticesForAlignment(m_pFaceAlignment, m_cFaceVerticeCount, m_pFaceVertices);
        // 解锁
        m_muxFaceVertice.unlock();
    }
    // 更新DX端数据
    if (SUCCEEDED(hr)){
        memcpy(
            m_SceneRenderer.face_model.face_mash_vertexs,
            m_pFaceVertices,
            sizeof(DirectX::XMFLOAT3) * m_cFaceVerticeCount
            );
        m_SceneRenderer.RefreshFaceData();
    }
    // 安全释放
    SafeRelease(pHDFaceFrame);

}