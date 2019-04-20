#include "stdafx.h"
#include "included.h"


// SceneRenderer 类 构造函数
SceneRenderer::SceneRenderer(){
    // 初始化呈现参数 脏矩形....好高级
    m_parameters.DirtyRectsCount = 0U;
    m_parameters.pDirtyRects = nullptr;
    m_parameters.pScrollOffset = nullptr;
    m_parameters.pScrollRect = nullptr;

    // 设备无关资源
    m_hrDIR = this->CreateDeviceIndependentResources();

}

// SceneRenderer 类 析构函数
SceneRenderer::~SceneRenderer(){
    this->DiscardDeviceResources();
#ifdef USING_DIRECT2D
    SafeRelease(m_pStandardTF);
    SafeRelease(m_pDWriteFactory);
    SafeRelease(m_pD2DFactory);
#endif
#ifdef _DEBUG
    if (this->debug_info){
        delete[] this->debug_info;
        this->debug_info = nullptr;
    }
#endif
}

// 创建设备无关资源
HRESULT SceneRenderer::CreateDeviceIndependentResources(){
    HRESULT hr = S_OK;
#ifdef USING_DIRECT2D
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
#endif
    return hr;
}

// SceneRenderer 类 创建设备相关资源
HRESULT SceneRenderer::CreateDeviceResources(){
    HRESULT hr = S_OK;
    // DXGI 设备
    IDXGIDevice1*                       pDxgiDevice = nullptr;
    // DXGI 适配器
    IDXGIAdapter*                       pDxgiAdapter = nullptr;
    // DXGI 工厂
    IDXGIFactory2*                      pDxgiFactory = nullptr;
    // 后备缓冲帧
    ID3D11Texture2D*                    pBackBuffer = nullptr;
    // 后备缓冲帧
    IDXGISurface*                       pDxgiBackBuffer = nullptr;
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
        hr = this->scene_model.Create(m_pD3DDevice);
    }
#pragma endregion
#ifdef USING_DIRECT2D
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
#endif

    SafeRelease(pDxgiDevice);
    SafeRelease(pDxgiAdapter);
    SafeRelease(pDxgiFactory);
    SafeRelease(pBackBuffer);
    SafeRelease(pDxgiBackBuffer);
    return hr;
}


// 丢弃设备资源
void SceneRenderer::DiscardDeviceResources(){
#ifdef USING_DIRECT2D
    SafeRelease(m_pD2DTargetBimtap);
    SafeRelease(m_pStandardBrush);
    SafeRelease(m_pD2DDeviceContext);
    SafeRelease(m_pD2DDevice);
#endif

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
    const float radius_base = 2.5f;
    // 计算Y值
    float float_y = this->y;
    if (float_y > DirectX::XM_PIDIV2) float_y = DirectX::XM_PIDIV2;
    else if (float_y < -DirectX::XM_PIDIV2) float_y = -DirectX::XM_PIDIV2;
    this->y = float_y;
    // 计算半径
    const float radius = radius_base * this->z * cosf(float_y);
    // 更新相机位置
    DirectX::XMVECTOR position = { radius* sinf(this->x), radius_base * this->z * sinf(float_y), radius_base + radius* cosf(this->x), 1.f };

    //
    DirectX::XMVECTOR look_at = { 0.f, 0.f, radius_base, 1.f };
    DirectX::XMVECTOR up = { 0.f, 1.f, 0.f, 1.f };
    auto mat = DirectX::XMMatrixLookAtLH(position, look_at, up);
    DirectX::XMStoreFloat4x4(
        &scene_model.vscb.view,
        mat
        );
    scene_model.RefreshCB(m_pD3DDeviceContext);
    // 清理颜色
#if 0
    FLOAT color[] = { 
        static_cast<float>(0x66) / 255.f,
        static_cast<float>(0xCC) / 255.f,
        static_cast<float>(0xFF) / 255.f,
        1.f
    }; 
#else
    FLOAT color[] = { 0.f, 0.f, 0.f, 1.f };
#endif
    // 清理
    m_pD3DDeviceContext->ClearRenderTargetView(m_pD3DRTView, color);
    m_pD3DDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    // 渲染面部
    scene_model.Render(m_pD3DDeviceContext);
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

}



// D3D11 设备丢失
HRESULT SceneRenderer::handle_device_lost(){
    // 这里就不处理了
    return S_OK;
}




// SceneRenderer::SceneModel 构造
SceneRenderer::SceneModel::SceneModel(){
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
    // 初始化
    if (m_pScenePoints){
        ZeroMemory(m_pScenePoints, sizeof(CameraSpacePoint)*c_cVertexCount);
    }
}

// SceneRenderer::SceneModel 创造
HRESULT SceneRenderer::SceneModel::Create(ID3D11Device* device){
    //if (!device) return ;
    D3D11_BUFFER_DESC buffer_desc = { 0 };
    buffer_desc.ByteWidth = this->c_cVertexCount * sizeof(CameraSpacePoint);
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
    // 读取顶点着色器CSO文件
    if (SUCCEEDED(hr)){
        hr = FileLoader.ReadFile(L"SimpleVS.cso") ? S_OK : STG_E_FILENOTFOUND;
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
    // 读取像素着色器CSO文件
    if (SUCCEEDED(hr)){
        hr = FileLoader.ReadFile(L"SimplePS.cso") ? S_OK : STG_E_FILENOTFOUND;
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


// SceneRenderer::SceneModel 渲染
void SceneRenderer::SceneModel::Render(ID3D11DeviceContext* context){
    //if (!context) return;
    // ---- 装配顶点
    // 设置顶点缓冲跨度和偏移.
    UINT offset = 0;
    UINT stride = sizeof(CameraSpacePoint);
    // IA: 输入本实例的顶点缓存
    context->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    // 拓扑结构: 点
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    // ---- 装配着色器缓存
    // 设置顶点着色器常量缓存 写入常量缓存寄存器0 (b0)
    context->VSSetConstantBuffers(0, 1, &m_pVSCBuffer);
    // ---- 装配着色器
    // 设置布局
    context->IASetInputLayout(m_pInputLayout);
    // 设置VS
    context->VSSetShader(m_pVertexShader, nullptr, 0);
    // 设置PS
    context->PSSetShader(m_pPixelShader, nullptr, 0);
    // ---- 渲染
    context->Draw(c_cVertexCount, 0);
}


// 更新常量缓存数据
void SceneRenderer::SceneModel::RefreshCB(ID3D11DeviceContext* context){
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

}


// 刷新顶点缓存
void SceneRenderer::SceneModel::RefreshVB(ID3D11DeviceContext* context){

    D3D11_MAPPED_SUBRESOURCE mapped_subsource;
    // 更新顶点缓存
    HRESULT hr = context->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subsource);

    // 写入数据
    if (SUCCEEDED(hr)){
        // 写入数据
        memcpy(mapped_subsource.pData, m_pScenePoints, sizeof(CameraSpacePoint)*this->c_cVertexCount);
        // 结束映射
        context->Unmap(m_pVertexBuffer, 0);
    }
}