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
    // 渲染窗口
    static void Render(ThisApp* pThis);
	// 消息循环
    void RunMessageLoop();
    // 刷新
    void Update();
private:
	// 窗口过程函数
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查彩色帧
    void check_color_frame();
    // 检查骨骼帧
    void check_body_frame();
private:
	// 窗口句柄
	HWND			            m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*              m_pKinect = nullptr;
    // 坐标映射器
    ICoordinateMapper*          m_pCoordinateMapper = nullptr;
    // 彩色帧读取器
    IColorFrameReader*          m_pColorFrameReader = nullptr;
    // 骨骼帧读取器
    IBodyFrameReader*           m_pBodyFrameReader = nullptr;
    // 彩色帧缓存
    RGBQUAD*                    m_pColorFrameBuffer = nullptr;
    // 退出
    BOOL                        m_bExit = FALSE;
	// 渲染器
	ImageRenderer	            m_ImagaRenderer;
    // 消息处理线程
    std::thread                 m_threadMSG;
};