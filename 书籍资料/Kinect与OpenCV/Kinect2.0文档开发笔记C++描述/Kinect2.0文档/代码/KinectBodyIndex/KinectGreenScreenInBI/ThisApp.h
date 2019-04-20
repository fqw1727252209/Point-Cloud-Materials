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
    // 检查复源帧
    void check_color_frame();
private:
	// 窗口句柄
	HWND			            m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*              m_pKinect = nullptr;
    // 坐标映射器
    ICoordinateMapper*          m_pCoordinateMapper = nullptr;
    // 复源帧读取器
    IMultiSourceFrameReader*    m_pMultiSourceFrameReader = nullptr;
    // 复源临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE             m_hMultiSourceFrameArrived = 0;
    // 映射器改变事件
    WAITABLE_HANDLE             m_hCoordinateMapperChanged = 0;
    // 深度帧坐标
    ColorSpacePoint*            m_pColorCoordinates = nullptr;
    // 彩色帧缓存
    RGBQUAD*                    m_pColorFrameBuffer = nullptr;
	// 渲染器
	ImageRenderer	            m_ImagaRenderer;
};