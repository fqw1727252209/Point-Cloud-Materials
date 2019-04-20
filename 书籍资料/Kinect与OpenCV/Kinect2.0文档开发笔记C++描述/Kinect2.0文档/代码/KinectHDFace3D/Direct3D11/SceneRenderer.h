// SceneRenderer类 场景渲染器

#pragma once

// 近裁剪距离
#define SCREEN_NEAR_Z (0.01f)
// 远裁剪距离
#define SCREEN_FAR_Z (10.f)

// Kinect
#define IMAGE_WIDTH (1920)
#define IMAGE_HEIGHT (1080)




// 场景渲染
class SceneRenderer{
public:
    // 顶点-法线 模型
    struct VertexNormal{
        // 坐标
        DirectX::XMFLOAT3   pos;
    };
    // 顶点常量缓存
    struct MatrixBufferType {
        // 世界转换
        DirectX::XMFLOAT4X4   world;
        // 视角转换
        DirectX::XMFLOAT4X4   view;
        // 透视转换
        DirectX::XMFLOAT4X4   projection;
    };
    // 光照材质
    struct  LightMaterialBufferType  {
        //平行光方向
        DirectX::XMFLOAT3 lightDirection;
        //高光指数
        float shininess;
        //光源位置
        DirectX::XMFLOAT4 lightPosition;
        //光源颜色
        DirectX::XMFLOAT4 lightColor;
        //光源的环境光反射系数
        DirectX::XMFLOAT4 globalAmbient;
        //摄像机的位置
        DirectX::XMFLOAT4 cameraPosition;
        //材质的自发光
        DirectX::XMFLOAT4 Ke;
        //材质的环境光系数
        DirectX::XMFLOAT4 Ka;
        //材质的漫反射系数
        DirectX::XMFLOAT4 Kd;
        //材质的高光系数
        DirectX::XMFLOAT4 Ks;
    };
    // 面部模型
    class FaceModel{
    public:
        // 构造
        FaceModel();
        // 创造
        HRESULT Create(ID3D11Device* device);
        // 刷新常量缓存
        void RefreshCB(ID3D11DeviceContext* context);
        // 刷新顶点缓存
        void RefreshVB(ID3D11DeviceContext* context);
        // 析构
        ~FaceModel(){ 
            SafeRelease(m_pVertexBuffer);
            SafeRelease(m_pIndexBuffer);
            SafeRelease(m_pInputLayout);
            SafeRelease(m_pVSCBuffer);
            SafeRelease(m_pPSCBuffer);
            SafeRelease(m_pVertexShader);
            SafeRelease(m_pGeometryShader);
            SafeRelease(m_pPixelShader);
        }
        // 渲染
        void Render(ID3D11DeviceContext* context);
    private:
        // 模型顶点缓存
        ID3D11Buffer*           m_pVertexBuffer = nullptr;
        // 模型索引缓存
        ID3D11Buffer*           m_pIndexBuffer = nullptr;
        // 顶点输入布局
        ID3D11InputLayout*      m_pInputLayout = nullptr;
        // 顶点着色器 常量缓存
        ID3D11Buffer*           m_pVSCBuffer = nullptr;
        // 像素着色器 常量缓存
        ID3D11Buffer*           m_pPSCBuffer = nullptr;
        // 顶点着色器    
        ID3D11VertexShader*     m_pVertexShader = nullptr;
        // 几何着色器
        ID3D11GeometryShader*   m_pGeometryShader = nullptr;
        // 像素着色器
        ID3D11PixelShader*      m_pPixelShader = nullptr;
    public:
        // 顶点着色器常量缓存
        MatrixBufferType                    vscb;
        // 像素着色器常量缓存
        LightMaterialBufferType             pscb;
        // 顶点数量
        UINT                                vertex_count = 0;
        // 面部网格
        VertexNormal*                       face_mash_vertexs = nullptr;
        // 索引数量
        UINT32                              index_count = 0;
        // 索引
        UINT32*                             index = nullptr;
        // 收集状态
        FaceModelBuilderCollectionStatus    co_status = FaceModelBuilderCollectionStatus_MoreFramesNeeded;
        // 收集状态
        FaceModelBuilderCaptureStatus       ca_status = FaceModelBuilderCaptureStatus_GoodFrameCapture;
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
    // 写入数据
    void WriteBitmapData(RGBQUAD*, int, int);
    // 获取缓存
    __forceinline RGBQUAD* GetBuffer(){ return m_pColorRGBX; }
    // 刷新模型数据
    void RefreshFaceData(){
        face_model.RefreshVB(m_pD3DDeviceContext);
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
    // 帧缓存数据
    RGBQUAD*                            m_pColorRGBX = nullptr;
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
    // 缓存位图帧
    ID2D1Bitmap1*                       m_pDrawBitmap = nullptr;
    // 标准文本格式
    IDWriteTextFormat*                  m_pStandardTF = nullptr;
    // 标准笔刷
    ID2D1SolidColorBrush*               m_pStandardBrush = nullptr;
    // 所创设备特性等级
    D3D_FEATURE_LEVEL                   m_featureLevel;
    // 手动交换链参数
    DXGI_PRESENT_PARAMETERS             m_parameters;
public:
    // 面部模型
    FaceModel                           face_model;
#ifdef _DEBUG
public:
    // 调试信息
    WCHAR*                              debug_info = new WCHAR[2048];
    // 信息长度
    size_t                              info_length = 0;
#endif
};