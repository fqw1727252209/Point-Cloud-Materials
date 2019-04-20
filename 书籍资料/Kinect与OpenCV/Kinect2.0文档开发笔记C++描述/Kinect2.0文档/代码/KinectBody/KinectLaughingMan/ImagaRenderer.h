#pragma once
// Author  : DustPG
// License : MIT: see more in "License.txt"

// 用途: 对图形图像的抽象 负责对图像渲染


#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

#define WNDWIDTH 1280
#define WNDHEIGHT 720

#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424

// 笑面男状态
enum class LaughingMan{
    Offline = 0,
    Online,
};

struct LaughingManState
{
    // 状态
    LaughingMan man = LaughingMan::Offline;
    // 当前文字角度
    FLOAT angle = 0.f;
    // 预测缩放率
    FLOAT zoom = 1.f;
    // 当前屏幕坐标
    D2D1_SIZE_F pos = D2D1::SizeF();
    // 更新
    __forceinline void update(){ angle -= 2.f; if (angle < 0.f) angle += 360.f; }
};

// 图像渲染器
class ImageRenderer{
public:
    // 构造函数
    ImageRenderer();
    // 析构函数
    ~ImageRenderer();
    // 设置窗口句柄
    HRESULT SetHwnd(HWND hwnd){ m_hwnd = hwnd; return CreateResources(); }
public:
    // 渲染帧
    HRESULT OnRender(UINT syn);
    // 这是彩色帧位图
    HRESULT LoadData(void* data, int width, int height){
        if (m_pColorFrame){
            D2D1_RECT_U rect = { 0, 0, width, height };
            return m_pColorFrame->CopyFromMemory(
                &rect,
                data,
                width* sizeof(RGBQUAD)
                );
        }
        return S_FALSE;
    }
    // 读取文件
    static HRESULT ImageRenderer::LoadBitmapFromFile(
        ID2D1DeviceContext *pRenderTarget,
        IWICImagingFactory2 *pIWICFactory,
        PCWSTR uri,
        UINT destinationWidth,
        UINT destinationHeight,
        ID2D1Bitmap1 **ppBitmap
        );
private:
    // 创建资源
    HRESULT CreateResources();
    // 丢弃设备有关资源
    void DiscardDeviceResources();
    // 创建笑面男
    HRESULT CreateLaughingMan();
    // 刻画笑面男
    void DrawLaughingMan();
private:
    // D2D 工厂
    ID2D1Factory1*						m_pD2DFactory = nullptr;
    // D2D 设备
    ID2D1Device*						m_pD2DDevice = nullptr;
    // D2D 设备上下文
    ID2D1DeviceContext*					m_pD2DDeviceContext = nullptr;
    // WIC 工厂
    IWICImagingFactory2*				m_pWICFactory = nullptr;
    // DWrite工厂
    IDWriteFactory1*					m_pDWriteFactory = nullptr;
    // 字体渲染参数
    IDWriteRenderingParams*				m_pDWParams = nullptr;
    // DXGI 交换链
    IDXGISwapChain1*					m_pSwapChain = nullptr;
    // D2D 位图 储存当前显示的位图
    ID2D1Bitmap1*						m_pD2DTargetBimtap = nullptr;
    // D2D 刻画缓存区
    ID2D1Bitmap1*						m_pBufferBitmap = nullptr;
    // 彩色位图
    ID2D1Bitmap1*                       m_pColorFrame = nullptr;
    //  所创设备特性等级
    D3D_FEATURE_LEVEL					m_featureLevel;
    // 手动交换链
    DXGI_PRESENT_PARAMETERS				m_parameters;
    // 窗口句柄
    HWND								m_hwnd = nullptr;
    // DWrite文本渲染器
    PathTextRenderer*                   m_pTextRenderer = nullptr;
    // 笑面男文本布局
    IDWriteTextLayout*                  m_pTextLayoutLaughingMan = nullptr;
    // 动画路径
    ID2D1EllipseGeometry*               m_pTextAnimationPath = nullptr;
    // 笑面男路径蓝色部分
    ID2D1PathGeometry*                  m_pLaughingManGeometryBlue = nullptr;
    // 笑面男路径白色部分
    ID2D1PathGeometry*                  m_pLaughingManGeometryWhite = nullptr;
    // 笑面蓝
    ID2D1SolidColorBrush*               m_pLaughingBlueBrush = nullptr;
    // 笑面白
    ID2D1SolidColorBrush*               m_pLaughingWhiteBrush = nullptr;
    // 屏幕DPI
    FLOAT								m_dpi = 0.F;
public:
    LaughingManState                    m_men[BODY_COUNT];
};