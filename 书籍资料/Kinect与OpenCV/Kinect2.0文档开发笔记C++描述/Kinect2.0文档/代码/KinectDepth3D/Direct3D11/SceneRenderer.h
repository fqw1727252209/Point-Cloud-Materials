// SceneRenderer类 场景渲染器

#pragma once

// 近裁剪距离
#define SCREEN_NEAR_Z (0.01f)
// 远裁剪距离
#define SCREEN_FAR_Z (100.f)

// Kinect
//#define IMAGE_WIDTH (1920)
//#define IMAGE_HEIGHT (1080)
// Depth
#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424




// 场景渲染
class SceneRenderer{
public:
    // 顶点常量缓存
    struct MatrixBufferType {
        // 世界转换
        DirectX::XMFLOAT4X4   world;
        // 视角转换
        DirectX::XMFLOAT4X4   view;
        // 透视转换
        DirectX::XMFLOAT4X4   projection;
    };
    // 场景模型
    class SceneModel{
    public:
        // 构造
        SceneModel();
        // 创造
        HRESULT Create(ID3D11Device* device);
        // 刷新常量缓存
        void RefreshCB(ID3D11DeviceContext* context);
        // 刷新顶点缓存
        void RefreshVB(ID3D11DeviceContext* context);
        // 获取顶点缓存
        __forceinline CameraSpacePoint* GetVB(){ return m_pScenePoints; }
        // 获取顶点数量
        __forceinline const UINT GetVertexCount(){ return c_cVertexCount; }
        // 析构
        ~SceneModel(){
            SafeRelease(m_pVertexBuffer);
            SafeRelease(m_pInputLayout);
            SafeRelease(m_pVSCBuffer);
            SafeRelease(m_pVertexShader);
            SafeRelease(m_pPixelShader);
            if (m_pScenePoints){
                delete[] m_pScenePoints;
                m_pScenePoints = nullptr;
            }
        }
        // 渲染
        void Render(ID3D11DeviceContext* context);
    private:
        // 模型顶点缓存
        ID3D11Buffer*           m_pVertexBuffer = nullptr;
        // 顶点输入布局
        ID3D11InputLayout*      m_pInputLayout = nullptr;
        // 顶点着色器 常量缓存
        ID3D11Buffer*           m_pVSCBuffer = nullptr;
        // 顶点着色器    
        ID3D11VertexShader*     m_pVertexShader = nullptr;
        // 像素着色器
        ID3D11PixelShader*      m_pPixelShader = nullptr;
        // 顶点数量
        const UINT              c_cVertexCount = DEPTH_WIDTH * DEPTH_HEIGHT;
        // 面部网格
        CameraSpacePoint*       m_pScenePoints = new CameraSpacePoint[c_cVertexCount];
    public:
        // Vertex Shader CBuffer
        MatrixBufferType        vscb;
    };
public:
    // 构造函数
    SceneRenderer();
    // 析构函数
    ~SceneRenderer();
    // 渲染
    HRESULT OnRender();
    // 设置窗口句柄
    HRESULT SetHwnd(HWND hwnd){ m_hwnd = hwnd; return this->CreateDeviceResources(); }
    // 创建设备无关资源
    HRESULT CreateDeviceIndependentResources();
    // 创建设备相关资源
    HRESULT CreateDeviceResources();
    // 丢弃设备资源
    void DiscardDeviceResources();
    // 刷新数据
    void RefreshData(){
        scene_model.RefreshVB(m_pD3DDeviceContext);
    }
    // 获取 D3D 设备
    ID3D11Device* Get3DDevice(){
        if (m_pD3DDevice){
            m_pD3DDevice->AddRef();
        }
        return m_pD3DDevice;
    }
private:
    // 处理设备丢失
    HRESULT handle_device_lost();
    // 刻画信息
    void draw_info();
private:
    // 创建结果
    HRESULT                             m_hrDIR = S_OK;
    // 窗口句柄
    HWND                                m_hwnd = nullptr;
    // D3D11 设备
    ID3D11Device*                       m_pD3DDevice = nullptr;
    // D2D11 设备上下文
    ID3D11DeviceContext*                m_pD3DDeviceContext = nullptr;
    // DXGI 交换链
    IDXGISwapChain1*                    m_pSwapChain = nullptr;
    // D3D 渲染目标
    ID3D11RenderTargetView*             m_pD3DRTView = nullptr;
    // D3D 深度缓存
    ID3D11Texture2D*                    m_pT2DDepthBuffer = nullptr;
    // D3D 深度模板视图
    ID3D11DepthStencilView*             m_pDepthStencilView = nullptr;
    // 光栅化状态
    ID3D11RasterizerState*              m_pRasterizerState = nullptr;
#ifdef USING_DIRECT2D
    // DWrite 工厂
    IDWriteFactory1*                    m_pDWriteFactory = nullptr;
    // D2D 工厂
    ID2D1Factory1*                      m_pD2DFactory = nullptr;
    // D2D 设备
    ID2D1Device*                        m_pD2DDevice = nullptr;
    // D2D 设备上下文
    ID2D1DeviceContext*                 m_pD2DDeviceContext = nullptr;
    // D2D 渲染目标位图
    ID2D1Bitmap1*                       m_pD2DTargetBimtap = nullptr;
    // 标准文本格式
    IDWriteTextFormat*                  m_pStandardTF = nullptr;
    // 标准笔刷
    ID2D1SolidColorBrush*               m_pStandardBrush = nullptr;
#endif
    // 所创设备特性等级
    D3D_FEATURE_LEVEL                   m_featureLevel;
    // 手动交换链参数
    DXGI_PRESENT_PARAMETERS             m_parameters;
public:
    // 场景模型
    SceneModel                          scene_model;
    // X
    std::atomic<float>                  x = 0.f;
    // Y
    std::atomic<float>                  y = 0.f;
    // Z
    std::atomic<float>                  z = 1.f;
#ifdef _DEBUG
public:
    // 调试信息
    WCHAR*                              debug_info = new WCHAR[2048];
    // 信息长度
    size_t                              info_length = 0;
#endif
};