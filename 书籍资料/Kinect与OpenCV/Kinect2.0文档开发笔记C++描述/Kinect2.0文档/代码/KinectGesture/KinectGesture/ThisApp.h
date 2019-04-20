// ThisApp类 本程序的抽象

#pragma once
#define WNDWIDTH 640
#define WNDHEIGHT 480


#define GESTURES_SAVED 32
// 手势显示信息
struct GesturesInfo {
    // 浮点数据
    float       float_var[GESTURES_SAVED];
    // 布尔数据1
    BOOLEAN     bool1_var[GESTURES_SAVED];
    // 布尔数据2
    BOOLEAN     bool2_var[GESTURES_SAVED];
    // 数据位置
    UINT32      index;
    // 类型
    BOOL        type;
};

// ThisApp类
class ThisApp{
public:
    // 构造函数
    ThisApp(WCHAR* file_name);
    // 析构函数
    ~ThisApp();
    // 初始化
    HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
    // 消息循环
    void RunMessageLoop();
    // 获取文件名
    __forceinline const WCHAR* GetFileName(){ return m_szFileNameBuffer; }
    // 获取手势数量
    __forceinline UINT GetGestureSize(){ return m_cGestureSize; }
    // 获取手势
    __forceinline IGesture** const GetGestures(){ return m_ppGestures; }
    // 获取手势信息
    __forceinline const GesturesInfo* GetGestureInfo(UINT index){ return &(m_vGesturesInfo[index]); }
private:
    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查彩色帧
    void check_color_frame();
    // 检查深度帧
    void check_depth_frame();
    // 检查骨骼帧
    void check_body_frame();
    // 检查手势帧
    void check_gesture_frame();
    // 载入手势文件
    HRESULT load_gesture_database_file(const WCHAR*);
    // 释放手势
    void release_gesture_data();
private:
    // 窗口句柄
    HWND                                m_hwnd = nullptr;
    // 渲染器
    ImageRenderer                       m_ImagaRenderer;
    // Kinect v2 传感器
    IKinectSensor*                      m_pKinect = nullptr;
    // 彩色帧读取器
    IColorFrameReader*                  m_pColorFrameReader = nullptr;
    // 深度帧读取器
    IDepthFrameReader*                  m_pDepthFrameReader = nullptr;
    // 骨骼帧读取器
    IBodyFrameReader*                   m_pBodyFrameReader = nullptr;
    // 坐标映射器
    ICoordinateMapper*                  m_pCoordinateMapper = nullptr;
    // 手势数据库
    IVisualGestureBuilderDatabase*      m_pGestureDatabase = nullptr;
    // 手势帧读取器
    IVisualGestureBuilderFrameReader*   m_pGestureFrameReader = nullptr;
    // 手势帧源
    IVisualGestureBuilderFrameSource*   m_pGestureFrameSource = nullptr;
    // 骨骼数据
    IBody*                              m_apBodies[BODY_COUNT];
    // 手势指针数组
    IGesture**                          m_ppGestures = m_apGestures;
    // 手势指针数组缓存大小
    UINT                                m_cGesturesBufferSize = lengthof(m_apGestures);
    // 手势指针数组大小
    UINT                                m_cGestureSize = 0;
    // 数据显示信息
    std::vector<GesturesInfo>           m_vGesturesInfo = std::vector<GesturesInfo>(lengthof(m_apGestures));
    // 手势指针数组-缓存
    IGesture*                           m_apGestures[64];
    // 文件名字缓存
    WCHAR                               m_szFileNameBuffer[MAX_PATH];
};