// ImageRender类 主管图形图像渲染

#pragma once

#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424


#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

//
struct BodyInfo{
    Joint joints[JointType_Count];
    D2D1_POINT_2F jointPoints[JointType_Count];
    HandState leftHandState = HandState_Unknown;
    HandState rightHandState = HandState_Unknown;
};



class ThisApp;

// ImageRenderer
class ImageRenderer{
public:
    // 构造函数
    ImageRenderer(ThisApp* boss);
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
    // 设置骨骼数据
    __forceinline void SetBodyInfo(int i, BodyInfo* info){ memcpy(m_bodyInfo + i, info, sizeof(BodyInfo)); }
    // 获取彩色帧缓存
    __forceinline RGBQUAD* GetColorBuffer(){ return m_pColorRGBX; }
    // 获取深度帧缓存
    __forceinline RGBQUAD* GetDepthBuffer(){ return m_pDepthRGBX; }
    // 写入数据
    void WriteBitmapData(RGBQUAD*,int, int, int);
private:
    // 创建设备无关资源
    HRESULT CreateDeviceIndependentResources();
    // 创建设备有关资源
    HRESULT CreateDeviceResources();
    // 丢弃设备有关资源
    void DiscardDeviceResources();
    // 从文件读取位图
    HRESULT LoadBitmapFromFile(ID2D1RenderTarget*, IWICImagingFactory *, PCWSTR uri, UINT, UINT, ID2D1Bitmap **);
    // 刻画身体
    void DrawBody();
    // 刻画手势
    void DrawGestureInfo();
    // 刻画身体
    void DrawBody(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints);
    // 刻画手
    void DrawHand(HandState handState, const D2D1_POINT_2F& handPosition);
    // 刻画骨骼
    void DrawBone(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints, JointType joint0, JointType joint1);
public:
    // 偏移量
    float                               show_offset = 0.f;
private:
    // 初始化结果
    HRESULT                             m_hrInit = E_FAIL;
    // 上司
    ThisApp*                            m_pBoss = nullptr;
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
    // 白色笔刷
    ID2D1Brush*                         m_pWhiteBrush = nullptr;
    // 关节被追踪的颜色
    ID2D1SolidColorBrush*               m_pBrushJointTracked = nullptr;
    // 关节被推断的颜色
    ID2D1SolidColorBrush*               m_pBrushJointInferred = nullptr;
    // 骨骼被追踪的颜色
    ID2D1SolidColorBrush*               m_pBrushBoneTracked = nullptr;
    // 骨骼被推断的颜色
    ID2D1SolidColorBrush*               m_pBrushBoneInferred = nullptr;
    // 手打开的颜色
    ID2D1SolidColorBrush*               m_pBrushHandClosed = nullptr;
    // 握拳的颜色
    ID2D1SolidColorBrush*               m_pBrushHandOpen = nullptr;
    // 部分手指伸开的颜色
    ID2D1SolidColorBrush*               m_pBrushHandLasso = nullptr;
    // 彩色帧缓存数据
    RGBQUAD*                            m_pColorRGBX = nullptr;
    // 彩色帧位图
    ID2D1Bitmap*                        m_pColorBitmap = nullptr;
    // 深度帧缓存数据
    RGBQUAD*                            m_pDepthRGBX = nullptr;
    // 深度帧位图
    ID2D1Bitmap*                        m_pDepthBitmap = nullptr;
    // 计时器
    PrecisionTimer                      m_timer;
    // FPS
    FLOAT                               m_fFPS = 0.f;
    // 骨骼数据
    BodyInfo                            m_bodyInfo[BODY_COUNT];
};