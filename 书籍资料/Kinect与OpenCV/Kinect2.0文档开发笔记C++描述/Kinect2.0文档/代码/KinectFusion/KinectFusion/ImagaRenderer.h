// ImageRender类 主管图形图像渲染

#pragma once

#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424

enum class EnumBitmapIndex{
    Index_Depth = 0,    // 深度图像
    Index_Surface,      // 表面
    Index_Normal,       // 法线
};

class ImageRenderer{
public:
    // 构造函数
    ImageRenderer();
    // 析构函数
    ~ImageRenderer();
    // 渲染
    HRESULT OnRender();
    // 设置窗口句柄
    void SetHwnd(HWND hwnd){ m_hwnd = hwnd; }
    // 改变大小
    void OnSize(UINT width, UINT height){
        if (!m_pRenderTarget)return;
        D2D1_SIZE_U size = {
            width, height
        };
        m_pRenderTarget->Resize(size);
    }
    // 返回初始化结果
    operator HRESULT() const{ return m_hrInit; }
    // 获取缓存
    __forceinline RGBQUAD* GetBuffer(){ return m_pColorRGBX; }
    // 写入数据
    void WriteBitmapData(EnumBitmapIndex, RGBQUAD*,int, int);
private:
    // 创建设备无关资源
    HRESULT CreateDeviceIndependentResources();
    // 创建设备有关资源
    HRESULT CreateDeviceResources();
    // 丢弃设备有关资源
    void DiscardDeviceResources();
    // 从文件读取位图
    HRESULT LoadBitmapFromFile(ID2D1RenderTarget*, IWICImagingFactory *, PCWSTR uri, UINT, UINT, ID2D1Bitmap **);
public:
    // D2D转换矩阵
    D2D1_MATRIX_3X2_F                   matrix = D2D1::Matrix3x2F::Identity();
private:
    // 初始化结果
    HRESULT                             m_hrInit = E_FAIL;
    // 窗口句柄
    HWND                                m_hwnd = nullptr;
    // D2D 工厂
    ID2D1Factory*                       m_pD2DFactory = nullptr;
    // WIC 工厂
    IWICImagingFactory*                 m_pWICFactory = nullptr;
    // DWrite工厂
    IDWriteFactory*                     m_pDWriteFactory = nullptr;
    // 正文文本渲染格式
    IDWriteTextFormat*                  m_pTextFormatMain = nullptr;
    // D2D渲染目标
    ID2D1HwndRenderTarget*              m_pRenderTarget = nullptr;
    // 帧缓存数据
    RGBQUAD*                            m_pColorRGBX = nullptr;
    // 缓存位图帧
    ID2D1Bitmap*                        m_pDrawBitmap = nullptr;
    // 表面位图帧
    ID2D1Bitmap*                        m_pSurfaceBitmap = nullptr;
    // 法线位图帧
    ID2D1Bitmap*                        m_pNormalBitmap = nullptr;
    // 白色笔刷
    ID2D1Brush*                         m_pMainBrush = nullptr;
    // 计时器
    PrecisionTimer                      m_timer;
    // FPS
    FLOAT                               m_fFPS = 0.f;
public:
    // 错误信息
    const WCHAR*                        error_info = L"";
    // 剖析信息
    WCHAR                               profiler_info[2048];
};