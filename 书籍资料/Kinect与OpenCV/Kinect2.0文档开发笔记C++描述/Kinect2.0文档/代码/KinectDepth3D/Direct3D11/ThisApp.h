// ThisApp类 本程序的抽象

#pragma once
#define TITLE L"Title"

#define WNDWIDTH 1024
#define WNDHEIGHT 768



// ThisApp类
class ThisApp {
public:
    // 构造函数
    ThisApp();
    // 析构函数
    ~ThisApp();
    // 渲染窗口
    static void Render(ThisApp* pThis);
    // 初始化
    HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
    // 消息循环
    void RunMessageLoop();
    // 刷新
    void Update();
private:
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查深度帧
    void check_depth_frame();
    // 初始化游戏对象
    void initialize_game_objects();
    // 反初始化游戏对象
    void finalize_game_objects();
    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
    // 渲染器
    SceneRenderer                       m_SceneRenderer;
    // 渲染线程
    std::thread                         m_threadRender;
    // 窗口句柄
    HWND                                m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*                      m_pKinect = nullptr;
    // 深度帧读取器
    IDepthFrameReader*                  m_pDepthFrameReader = nullptr;
    // 坐标映射器
    ICoordinateMapper*                  m_pMapper = nullptr;
    // 退出
    std::atomic<BOOL>                   m_bExit = FALSE;
    // 上帧鼠标X位置
    int                                 m_lastFrameX = 0;
    // 上帧鼠标Y位置
    int                                 m_lastFrameY = 0;
};
