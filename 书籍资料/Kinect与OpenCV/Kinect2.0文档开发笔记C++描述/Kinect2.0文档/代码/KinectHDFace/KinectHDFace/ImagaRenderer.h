// ImageRender类 主管图形图像渲染

#pragma once

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

struct FaceData{
    // 被跟踪?
    UINT32              tracked;
    // 动画单元
    float               au[FaceShapeAnimations_Count];
    // 面部外框
    RectI               face_box;
    // 面部旋转四元数
    Vector4             face_rotation;
    // 颈部旋转四中心
    union{
        D2D1_POINT_2F       head_pivot_1;
        ColorSpacePoint     head_pivot_2;
    };
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
	// 返回初始化结果
	operator HRESULT() const{ return m_hrInit; }
    // 获取缓存
    __forceinline RGBQUAD* GetBuffer(){ return m_pColorRGBX; }
    // 写入数据
    void WriteBitmapData(RGBQUAD*, int, int);
private:
    // 刻画面部数据
    void draw_facedata();
public:
    // 旋转四元数 转换成 旋转角度
    static void ExtractFaceRotationInDegrees(const Vector4* pQuaternion, float* pPitch, float* pYaw, float* pRoll);
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
    HRESULT								m_hrInit = E_FAIL;
	// 窗口句柄
    HWND								m_hwnd = nullptr;
	// D2D 工厂
    ID2D1Factory*						m_pD2DFactory = nullptr;
	// WIC 工厂
    IWICImagingFactory*					m_pWICFactory = nullptr;
	// DWrite工厂
    IDWriteFactory*						m_pDWriteFactory = nullptr;
	// 正文文本渲染格式
    IDWriteTextFormat*					m_pTextFormatMain = nullptr;
    // 显示文本渲染格式
    IDWriteTextFormat*					m_pTextFormatShow = nullptr;
	// D2D渲染目标
    ID2D1HwndRenderTarget*				m_pRenderTarget = nullptr;
    // 帧缓存数据
    RGBQUAD*                            m_pColorRGBX = nullptr;
    // 缓存位图帧
    ID2D1Bitmap*                        m_pDrawBitmap = nullptr;
    // 白色笔刷
    ID2D1SolidColorBrush*               m_pWhiteBrush = nullptr;
    // 计时器
    PrecisionTimer                      m_timer;
    // FPS
    FLOAT                               m_fFPS = 0.f;
public:
    // 数据
    FaceData                            data;
};