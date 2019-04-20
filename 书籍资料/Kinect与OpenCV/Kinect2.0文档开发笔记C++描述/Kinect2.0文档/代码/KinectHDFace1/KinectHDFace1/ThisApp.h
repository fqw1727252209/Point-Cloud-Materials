// ThisApp类 本程序的抽象

#pragma once
#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

// ThisApp类
class ThisApp
{
public:
	// 构造函数
	ThisApp();
	// 析构函数
	~ThisApp();
	// 初始化
	HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
	// 消息循环
	void RunMessageLoop();
private:
	// 窗口过程函数
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查彩色帧
    void check_color_frame();
    // 检查骨骼帧
    void check_body_frame();
    // 检查高清面部
    void check_hd_face_frame();
private:
	// 窗口句柄
	HWND			                    m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*                      m_pKinect = nullptr;
    // 彩色帧读取器
    IColorFrameReader*                  m_pColorFrameReader = nullptr;
    // 高清面部帧源
    IHighDefinitionFaceFrameSource*     m_pHDFaceFrameSource = nullptr;
    // 高清面部帧读取器
    IHighDefinitionFaceFrameReader*     m_pHDFaceFrameReader = nullptr;
    // 面部特征对齐
    IFaceAlignment*                     m_pFaceAlignment = nullptr;
    // 骨骼帧读取器
    IBodyFrameReader*                   m_pBodyFrameReader = nullptr;
    // 面部模型
    IFaceModel*                         m_pFaceModel = nullptr;
    // 坐标映射器
    ICoordinateMapper*                  m_pMapper = nullptr;
    // 面部模型顶点
    CameraSpacePoint*                   m_pFaceVertices = nullptr;
    // 面部模型顶点数量
    UINT                                m_cFaceVerticeCount = 0;
    // 保留 保证在64位程序下以 64位 对齐
    UINT                                unused = 0;
    // 彩色临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE                     m_hColorFrameArrived = 0;
    // 骨骼临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE                     m_hBodyFrameArrived = 0;
    // 高清面部临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE                     m_hHDFFrameArrived = 0;
	// 渲染器
	ImageRenderer	                    m_ImagaRenderer;
};