// ImageRender类 主管图形图像渲染

#pragma once

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080


struct FaceData {
    // 是否被跟踪
    BOOL            tracked;
    // 面部外框
    RectI           faceBox;
    // 面部特征点
    PointF          facePoints[FacePointType::FacePointType_Count];
    // 面部旋转四元数
    Vector4         faceRotation;
    // 面部相关属性
    DetectionResult faceProperties[FaceProperty::FaceProperty_Count];
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
    // 刻画面部
    void draw_face();
public:
	// 创建设备无关资源
	HRESULT CreateDeviceIndependentResources();
	// 创建设备有关资源
	HRESULT CreateDeviceResources();
	// 丢弃设备有关资源
	void DiscardDeviceResources();
    // 将选择四元数转换成旋转角度
    static void ExtractFaceRotationInDegrees(const Vector4* pQuaternion, float* pPitch, float* pYaw, float* pRoll);
	// 从文件读取位图
    static HRESULT LoadBitmapFromFile(ID2D1RenderTarget*, IWICImagingFactory *, PCWSTR uri, UINT, UINT, ID2D1Bitmap **);
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
	// D2D渲染目标
    ID2D1HwndRenderTarget*				m_pRenderTarget = nullptr;
    // 帧缓存数据
    RGBQUAD*                            m_pColorRGBX = nullptr;
    // 缓存位图帧
    ID2D1Bitmap*                        m_pDrawBitmap = nullptr;
    // 白色笔刷
    ID2D1Brush*                         m_pWhiteBrush = nullptr;
    // 计时器
    PrecisionTimer                      m_timer;
    // FPS
    FLOAT                               m_fFPS = 0.f;
public:
    // 面部数据
    FaceData                            face_data[BODY_COUNT];
};