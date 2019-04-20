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
    // 检查红外帧
    void check_infrared_frame();
private:
	// 窗口句柄
	HWND			        m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*          m_pKinect = nullptr;
    // 红外帧读取器
    IInfraredFrameReader*   m_pInfraredFrameReader = nullptr;
    // 红外临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE         m_hInfraredFrameArrived = 0;
	// 渲染器
	ImageRenderer	        m_ImagaRenderer;
};