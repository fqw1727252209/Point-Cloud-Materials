#include "stdafx.h"
#include "included.h"

// D3D11 渲染管线
// * 1. IA:  Input Assembler Stage 输入装配阶段
// *    



// SceneRenderer 类 构造函数
SceneRenderer::SceneRenderer(){
    // 初始化呈现参数 脏矩形....好高级
    m_parameters.DirtyRectsCount = 0U;
    m_parameters.pDirtyRects = nullptr;
    m_parameters.pScrollOffset = nullptr;
    m_parameters.pScrollRect = nullptr;

    // 设备无关资源
    m_hrDIR = this->CreateDeviceIndependentResources();

    // 创建缓存
    if (SUCCEEDED(m_hrDIR)){
        // 创建缓冲区
        m_pColorRGBX = new RGBQUAD[IMAGE_WIDTH*IMAGE_HEIGHT];
        if (!m_pColorRGBX) m_hrDIR = E_OUTOFMEMORY;
    }
}

// SceneRenderer 类 析构函数
SceneRenderer::~SceneRenderer(){
    this->DiscardDeviceResources();
    SafeRelease(m_pStandardTF);
    SafeRelease(m_pDWriteFactory);
    SafeRelease(m_pD2DFactory);
#ifdef _DEBUG
    if (this->debug_info){
        delete[] this->debug_info;
        this->debug_info = nullptr;
    }
#endif
    if (m_pColorRGBX){
        delete[] m_pColorRGBX;
        m_pColorRGBX = nullptr;
    }
}

// 创建设备无关资源
HRESULT SceneRenderer::CreateDeviceIndependentResources(){
    HRESULT hr = S_OK;
    // 创建DirectWrite工厂
    if (SUCCEEDED(hr)){
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
            );
    }
    // 创建 D2D 工厂
    if (SUCCEEDED(hr)) {
        hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_PPV_ARGS(&m_pD2DFactory)
            );

    }
    // 创建标准文本格式
    if (SUCCEEDED(hr)){
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            23.3333f * 2.33333f,
            L"",
            &m_pStandardTF
            );
    }
    // 居中对齐
    if (SUCCEEDED(hr)){
        m_pStandardTF->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_pStandardTF->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    return hr;
}

// SceneRenderer 类 创建设备相关资源
HRESULT SceneRenderer::CreateDeviceResources(){
    HRESULT hr = S_OK;
    // DXGI 设备
    IDXGIDevice1*                        pDxgiDevice = nullptr;
    // DXGI 适配器
    IDXGIAdapter*                        pDxgiAdapter = nullptr;
    // DXGI 工厂
    IDXGIFactory2*                        pDxgiFactory = nullptr;
    // 后备缓冲帧
    ID3D11Texture2D*                    pBackBuffer = nullptr;
    // 后备缓冲帧
    IDXGISurface*                        pDxgiBackBuffer = nullptr;
#pragma region 初始化 Direct3D
    // 创建D3D设备 可以直接使用D3D11CreateDeviceAndSwapChain
    if (SUCCEEDED(hr)){
#ifdef USING_DIRECT2D
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
        UINT creationFlags = 0;
#endif
#ifdef _DEBUG
        // Debug状态 有D3D DebugLayer就取消注释
        //creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] =  {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            creationFlags,
            featureLevels,
            lengthof(featureLevels),
            D3D11_SDK_VERSION,
            &m_pD3DDevice,
            &m_featureLevel,
            &m_pD3DDeviceContext
            );
        // 提示显卡性能
        if (FAILED(hr)){
            ::MessageBoxW(m_hwnd, L"该显示适配器不支持DX11", L"DX11", MB_OK);
        }
    }
    // 获取 DXGI 设备
    if (SUCCEEDED(hr)) {
        hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
    }
    // 获取Dxgi 适配器
    if (SUCCEEDED(hr)) {
        hr = pDxgiDevice->GetAdapter(&pDxgiAdapter);
    }
    // 获取Dxgi 工厂
    if (SUCCEEDED(hr)) {
        hr = pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory));
    }
    // 禁止 Alt + Enter 全屏
    if (SUCCEEDED(hr)){
        hr = pDxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    }
    // 获取窗口大小
    RECT rect;
    ::GetClientRect(m_hwnd, &rect);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    // 创建窗口交换链
    if (SUCCEEDED(hr))  {
        // 交换链描述体
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = rect.right;
        swapChainDesc.Height = rect.bottom;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        swapChainDesc.BufferCount = 2;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = 0;

        hr = pDxgiFactory->CreateSwapChainForHwnd(
            m_pD3DDevice,
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &m_pSwapChain
            );
    }
    // 获取缓冲帧 Texeture2D
    if (SUCCEEDED(hr)){
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    }
    // 创建渲染目标
    if (SUCCEEDED(hr)){
        hr = m_pD3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pD3DRTView);
    }
    // 创建深度缓冲
    if (SUCCEEDED(hr)){
        // 初始化深度缓冲描述.
        D3D11_TEXTURE2D_DESC depthBufferDesc = { 0 };
        // 设置深度缓冲描述
        depthBufferDesc.Width = rect.right;
        depthBufferDesc.Height = rect.bottom;
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.ArraySize = 1;
        depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.SampleDesc.Quality = 0;
        depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthBufferDesc.CPUAccessFlags = 0;
        depthBufferDesc.MiscFlags = 0;
        // 创建深度缓存
        hr = m_pD3DDevice->CreateTexture2D(
            &depthBufferDesc, 
            nullptr, 
            &m_pT2DDepthBuffer
            );
    }
    // 创建深度模版视图
    if (SUCCEEDED(hr)){
        hr = m_pD3DDevice->CreateDepthStencilView(
            m_pT2DDepthBuffer, 
            nullptr, 
            &m_pDepthStencilView);
    }
    // 设置该缓冲帧(交换链与深度)为输出目标 OM =  Output-Merger Stage 合并输出阶段
    if (SUCCEEDED(hr)){
        m_pD3DDeviceContext->OMSetRenderTargets(1, &m_pD3DRTView, m_pDepthStencilView);
    }
    // 设置
    if (SUCCEEDED(hr)){
        D3D11_RASTERIZER_DESC rasterDesc;
        // 设置光栅化描述，指定多边形如何被渲染.
        rasterDesc.AntialiasedLineEnable = false;
        rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = true;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = false;
        rasterDesc.MultisampleEnable = false;
        rasterDesc.ScissorEnable = false;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        // 创建光栅化状态
        hr = m_pD3DDevice->CreateRasterizerState(&rasterDesc, &m_pRasterizerState);
    }
    // 并设为当前的光栅化状态 RS =  Rasterizer Stage  光栅化阶段
    if (SUCCEEDED(hr)){
        m_pD3DDeviceContext->RSSetState(m_pRasterizerState);
    }
    // 设置视口
    if (SUCCEEDED(hr)){
        D3D11_VIEWPORT viewPort = { 0 };
        viewPort.Width = static_cast<FLOAT>(rect.right);
        viewPort.Height = static_cast<FLOAT>(rect.bottom);
        viewPort.MinDepth = 0.f;
        viewPort.MaxDepth = 1.f;
        viewPort.TopLeftX = 0.f;
        viewPort.TopLeftY = 0.f;
        m_pD3DDeviceContext->RSSetViewports(1, &viewPort);
    }
    // 创建模型
    if (SUCCEEDED(hr)){
        hr = this->face_model.Create(m_pD3DDevice);
    }
#pragma endregion

#pragma region 初始化 Direct2D
    // 获取缓冲帧 DXGI Surface
    if (SUCCEEDED(hr)){
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
    }
    // 创建D2D设备
    if (SUCCEEDED(hr)) {
        hr = m_pD2DFactory->CreateDevice(pDxgiDevice, &m_pD2DDevice);
    }
    // 创建D2D设备上下文
    if (SUCCEEDED(hr)) {
        // 这时创建D2D设备
        hr = m_pD2DDevice->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &m_pD2DDeviceContext
            );
    }
    // 创建标准笔刷
    if (SUCCEEDED(hr)){
        hr = m_pD2DDeviceContext->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::DarkOrange),
            &m_pStandardBrush
            );
    }
    // 创建彩色帧
    if (SUCCEEDED(hr)){
        D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.f,
            96.f
            );
        hr = m_pD2DDeviceContext->CreateBitmap(
            D2D1::SizeU(IMAGE_WIDTH, IMAGE_HEIGHT),
            nullptr,
            0,
            &bitmapProperties,
            &m_pDrawBitmap
            );
    }
    // 创建D2D 位图
    if (SUCCEEDED(hr)){
        D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.f,
            96.f
            );
        // 创建
        hr = m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pD2DTargetBimtap
            );
        // 设置D2D渲染目标
        m_pD2DDeviceContext->SetTarget(m_pD2DTargetBimtap);
        // 设置像素为单位
        m_pD2DDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    }
#pragma endregion

    SafeRelease(pDxgiDevice);
    SafeRelease(pDxgiAdapter);
    SafeRelease(pDxgiFactory);
    SafeRelease(pBackBuffer);
    SafeRelease(pDxgiBackBuffer);
    return hr;
}


// 丢弃设备资源
void SceneRenderer::DiscardDeviceResources(){

    SafeRelease(m_pDrawBitmap);
    SafeRelease(m_pD2DTargetBimtap);
    SafeRelease(m_pStandardBrush);
    SafeRelease(m_pD2DDeviceContext);
    SafeRelease(m_pD2DDevice);

    SafeRelease(m_pRasterizerState);
    SafeRelease(m_pDepthStencilView);
    SafeRelease(m_pD3DRTView);
    SafeRelease(m_pT2DDepthBuffer);
    SafeRelease(m_pSwapChain);
    SafeRelease(m_pD3DDeviceContext);
    SafeRelease(m_pD3DDevice);
}

// 渲染
HRESULT SceneRenderer::OnRender(){
    // 清理颜色
    FLOAT color[] = { 
        static_cast<float>(0x66) / 255.f,
        static_cast<float>(0xCC) / 255.f,
        static_cast<float>(0xFF) / 255.f,
        1.f
    };
    // 清理
    m_pD3DDeviceContext->ClearRenderTargetView(m_pD3DRTView, color);
    m_pD3DDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    // 渲染底部彩色帧
    m_pD2DDeviceContext->BeginDraw();
    D2D1_RECT_F rect = { 0.f };
    reinterpret_cast<D2D1_SIZE_F&>(rect.right) = m_pD2DDeviceContext->GetSize();
    m_pD2DDeviceContext->DrawBitmap(
        m_pDrawBitmap, &rect
        );
    m_pD2DDeviceContext->EndDraw();

    // 渲染面部
    face_model.Render(m_pD3DDeviceContext);
    // 刻画信息
    draw_info();

    // 呈现
    //m_pSwapChain->Present1(1, 0, &this->m_parameters);
    HRESULT hr = m_pSwapChain->Present(1, 0);
    // 设备丢失?
    if (hr == DXGI_ERROR_DEVICE_REMOVED){
#ifdef _DEBUG
        _cwprintf(L"DXGI_ERROR_DEVICE_REMOVED: D3D11 设备丢失\n");
#endif
        // 处理设备丢失 
        hr = handle_device_lost();
    }
    return hr;
}



// 刻画信息
void SceneRenderer::draw_info(){
    m_pD2DDeviceContext->BeginDraw();
    D2D1_RECT_F rect = { 0 };
    reinterpret_cast<D2D1_SIZE_F&>(rect.right) = m_pD2DDeviceContext->GetSize();
    // 显示信息
    if (this->face_model.co_status == FaceModelBuilderCollectionStatus_Complete){
        m_pD2DDeviceContext->DrawText(
            L"面部信息信息搜集完毕, 感谢您的配合\n键入S键可保存这瞬间面部模型",
            lengthof(L"面部信息信息搜集完毕, 感谢您的配合\n键入S键可保存这瞬间面部模型") - 1,
            m_pStandardTF,
            rect,
            m_pStandardBrush
            );
    }
    // 汉字左右可能分不清楚， 所以追加LR字母
    else if (this->face_model.co_status & FaceModelBuilderCollectionStatus_LeftViewsNeeded){
        m_pD2DDeviceContext->DrawText(
            L"请向左(L)边看看, 方便搜集面部信息",
            lengthof(L"请向左(L)边看看, 方便搜搜集面部信息") - 1,
            m_pStandardTF,
            rect,
            m_pStandardBrush
            );
    }
    else if (this->face_model.co_status & FaceModelBuilderCollectionStatus_RightViewsNeeded){
        m_pD2DDeviceContext->DrawText(
            L"请向右(R)边看看, 方便搜搜集面部信息",
            lengthof(L"请向右(R)边看看, 方便搜搜集面部信息") - 1,
            m_pStandardTF,
            rect,
            m_pStandardBrush
            );
    }
    else if (this->face_model.co_status & FaceModelBuilderCollectionStatus_FrontViewFramesNeeded){
        m_pD2DDeviceContext->DrawText(
            L"请正视Kinect, 方便搜搜集面部信息",
            lengthof(L"请正视Kinect, 方便搜搜集面部信息") - 1,
            m_pStandardTF,
            rect,
            m_pStandardBrush
            );
    }
    else if (this->face_model.co_status & FaceModelBuilderCollectionStatus_TiltedUpViewsNeeded){
        m_pD2DDeviceContext->DrawText(
            L"请抬头向上看, 方便搜集面部信息",
            lengthof(L"请抬头向上看, 方便搜集面部信息") - 1,
            m_pStandardTF,
            rect,
            m_pStandardBrush
            );
    }
    m_pD2DDeviceContext->EndDraw();
}



// D3D11 设备丢失
HRESULT SceneRenderer::handle_device_lost(){
    // 这里就不处理了
    return S_OK;
}




// 写入数据
void SceneRenderer::WriteBitmapData(RGBQUAD* data, int width, int height){
    D2D1_RECT_U rect = { 0, 0, width, height };
    m_pDrawBitmap->CopyFromMemory(
        &rect,
        data,
        width* sizeof(RGBQUAD)
        );
}




// ceneRenderer::FaceModel 构造
SceneRenderer::FaceModel::FaceModel(){
    //  ---- 设置转换矩阵
    DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
    // 世界转换
    DirectX::XMStoreFloat4x4(
        &this->vscb.world,
        mat
        );

    // 视角视角转换
    DirectX::XMVECTOR position = { 0.f, 0.f, 0.f, 1.f };
    DirectX::XMVECTOR look_at = { 0.f, 0.f, 1.f, 1.f };
    DirectX::XMVECTOR up = { 0.f, 1.f, 0.f, 1.f };
    mat = DirectX::XMMatrixLookAtLH(position, look_at, up);
    DirectX::XMStoreFloat4x4(
        &this->vscb.view,
        mat
        );

    // ---- 设置材质
    pscb.Ke = DirectX::XMFLOAT4(0.8f, 0.0f, 0.2f, 1.0f);
    pscb.Ka = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    pscb.Kd = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    pscb.Ks = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    pscb.lightDirection = DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f);
    pscb.lightPosition = DirectX::XMFLOAT4(5.0f, 5.0f, -3.0f, 1.0f);
    pscb.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    pscb.globalAmbient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    pscb.cameraPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    pscb.shininess = 10.f;
}

// SceneRenderer::FaceModel 创造
HRESULT SceneRenderer::FaceModel::Create(ID3D11Device* device){
    //if (!device) return ;
    D3D11_BUFFER_DESC buffer_desc = { 0 };
    buffer_desc.ByteWidth = this->vertex_count * sizeof(SceneRenderer::VertexNormal);
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    // 每帧均不同 所以需要GPU可读 CPU可写
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    // 创建顶点缓存
    HRESULT hr = device->CreateBuffer(
        &buffer_desc,
        nullptr,
        &m_pVertexBuffer
        );
    // 创建索引缓存
    if (SUCCEEDED(hr)){
        buffer_desc = {
            index_count * sizeof(UINT32),
            D3D11_USAGE_IMMUTABLE,
            D3D11_BIND_INDEX_BUFFER,
            0,
            0,
            0
        };
        // 子资源数据
        D3D11_SUBRESOURCE_DATA ibData = { 0 };
        ibData.pSysMem = index;
        //根据描述和数据创建索引缓存
        hr = device->CreateBuffer(&buffer_desc, &ibData, &m_pIndexBuffer);
    }
    // 读取顶点着色器CSO文件
    if (SUCCEEDED(hr)){
        hr = FileLoader.ReadFile(L"SimpleLightingVS.cso") ? S_OK : STG_E_FILENOTFOUND;
    }
    // 创建顶点着色器
    if (SUCCEEDED(hr)){
        hr = device->CreateVertexShader(
            FileLoader.GetData(),
            FileLoader.GetLength(),
            nullptr,
            &m_pVertexShader
            );
    }
    // 顶点位置
    D3D11_INPUT_ELEMENT_DESC inputele_desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    // 创建输入布局
    if (SUCCEEDED(hr)){
        hr = device->CreateInputLayout(
            inputele_desc,
            lengthof(inputele_desc),
            FileLoader.GetData(),
            FileLoader.GetLength(),
            &m_pInputLayout
            );
    }
    // 读取几何着色器CSO文件
    if (SUCCEEDED(hr)){
        hr = FileLoader.ReadFile(L"SimpleLightingGS.cso") ? S_OK : STG_E_FILENOTFOUND;
    }
    // 创建几何着色器
    if (SUCCEEDED(hr)){
        hr = device->CreateGeometryShader(
            FileLoader.GetData(),
            FileLoader.GetLength(),
            nullptr,
            &m_pGeometryShader
            );
    }
    // 读取像素着色器CSO文件
    if (SUCCEEDED(hr)){
        hr = FileLoader.ReadFile(L"SimpleLightingPS.cso") ? S_OK : STG_E_FILENOTFOUND;
    }
    // 创建像素着色器
    if (SUCCEEDED(hr)){
        hr = device->CreatePixelShader(
            FileLoader.GetData(),
            FileLoader.GetLength(),
            nullptr,
            &m_pPixelShader
            );
    }
    // 创建VS常量缓存
    if (SUCCEEDED(hr)){
        // 缓存描述: 储存矩阵的常量(CPU写 GPU读)缓存
        buffer_desc = {
            sizeof(MatrixBufferType),
            D3D11_USAGE_DYNAMIC,
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_CPU_ACCESS_WRITE,
            0,
            0
        };
        // 创建
        hr = device->CreateBuffer(&buffer_desc, nullptr, &m_pVSCBuffer);
    }
    // 创建PS常量缓存
    if (SUCCEEDED(hr)){
        // 缓存描述: 储存光照的常量(CPU写 GPU读)缓存
        buffer_desc = {
            sizeof(LightMaterialBufferType),
            D3D11_USAGE_DYNAMIC,
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_CPU_ACCESS_WRITE,
            0,
            0
        };
        // 创建
        hr = device->CreateBuffer(&buffer_desc, nullptr, &m_pPSCBuffer);
    }
    // 顺带设置首次数据
    if (SUCCEEDED(hr)){
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context);
        if (device){
            this->RefreshCB(context);
            this->RefreshVB(context);
        }
        SafeRelease(context);
    }
    return hr;
}


// SceneRenderer::FaceModel 渲染
void SceneRenderer::FaceModel::Render(ID3D11DeviceContext* context){
    //if (!context) return;
    // ---- 装配顶点
    // 设置顶点缓冲跨度和偏移.
    UINT offset = 0;
    UINT stride = sizeof(SceneRenderer::VertexNormal);
    // IA: 输入本实例的顶点缓存
    context->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    // IA : 设置索引缓存
    context->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    // 拓扑结构: 三角链
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // ---- 装配着色器缓存
    // 设置顶点着色器常量缓存 写入常量缓存寄存器0 (b0)
    context->VSSetConstantBuffers(0, 1, &m_pVSCBuffer);
    // 设置像素着色器常量缓存 写入常量缓存寄存器0 (b0)
    context->PSSetConstantBuffers(0, 1, &m_pPSCBuffer);
    // 设置布局
    context->IASetInputLayout(m_pInputLayout);
    // 设置VS
    context->VSSetShader(m_pVertexShader, nullptr, 0);
    // 设置PS
    context->PSSetShader(m_pPixelShader, nullptr, 0);
    // ---- 装配着色器
    // 设置布局
    context->IASetInputLayout(m_pInputLayout);
    // 设置VS
    context->VSSetShader(m_pVertexShader, nullptr, 0);
    // 设置GS
    context->GSSetShader(m_pGeometryShader, nullptr, 0);
    // 设置PS
    context->PSSetShader(m_pPixelShader, nullptr, 0);
    // ---- 渲染
    context->DrawIndexed(index_count,0, 0);
}


// 更新数据
void SceneRenderer::FaceModel::RefreshCB(ID3D11DeviceContext* context){
    D3D11_MAPPED_SUBRESOURCE mapped_subsource;
    // 更新顶点着色器常量缓存
    HRESULT hr = context->Map(m_pVSCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subsource);
    
    // 写入数据
    if (SUCCEEDED(hr)){
        auto* data = reinterpret_cast<MatrixBufferType*>(mapped_subsource.pData);
        // D3D11 要求: 矩阵转置
        DirectX::XMMATRIX mat_temp;
        mat_temp = DirectX::XMLoadFloat4x4(&vscb.projection);
        mat_temp = DirectX::XMMatrixTranspose(mat_temp);
        DirectX::XMStoreFloat4x4(&data->projection, mat_temp);

        mat_temp = DirectX::XMLoadFloat4x4(&vscb.view);
        mat_temp = DirectX::XMMatrixTranspose(mat_temp);
        DirectX::XMStoreFloat4x4(&data->view, mat_temp);

        mat_temp = DirectX::XMLoadFloat4x4(&vscb.world);
        mat_temp = DirectX::XMMatrixTranspose(mat_temp);
        DirectX::XMStoreFloat4x4(&data->world, mat_temp);
        // 结束映射
        context->Unmap(m_pVSCBuffer, 0);
    }


    // 更新像素着色器常量
    hr = context->Map(m_pPSCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subsource);

    // 写入数据
    if (SUCCEEDED(hr)){
        // 写入数据
        memcpy(mapped_subsource.pData, &pscb, sizeof(pscb));
        // 结束映射
        context->Unmap(m_pPSCBuffer, 0);
    }

}


// 刷新顶点缓存
void SceneRenderer::FaceModel::RefreshVB(ID3D11DeviceContext* context){

    D3D11_MAPPED_SUBRESOURCE mapped_subsource;
    // 更新顶点缓存
    HRESULT hr = context->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subsource);

    // 写入数据
    if (SUCCEEDED(hr)){
        // 写入数据
        memcpy(mapped_subsource.pData, face_mash_vertexs, sizeof(VertexNormal)*this->vertex_count);
        // 结束映射
        context->Unmap(m_pVertexBuffer, 0);
    }
}