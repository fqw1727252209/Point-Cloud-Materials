// ThisApp类 本程序的抽象

#pragma once

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
    // 检查面部
    void check_faces();
private:
	// 窗口句柄
	HWND			        m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*          m_pKinect = nullptr;
    // 彩色帧读取器
    IColorFrameReader*      m_pColorFrameReader = nullptr;
    // 骨骼帧读取器
    IBodyFrameReader*       m_pBodyFrameReader = nullptr;
    // 彩色临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE         m_hColorFrameArrived = 0;
    // 骨骼临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE         m_hBodyFrameArrived = 0;
    // 面部帧源 因为需要读取跟踪数据 所以在这里保留
    IFaceFrameSource*		m_pFaceFrameSources[BODY_COUNT];
    // 面部帧读取器
    IFaceFrameReader*		m_pFaceFrameReaders[BODY_COUNT];
    // 骨骼数据
    IBody*                  m_pBodies[BODY_COUNT];
	// 渲染器
	ImageRenderer	        m_ImagaRenderer;
};