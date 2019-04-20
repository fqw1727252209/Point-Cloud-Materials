// ThisApp类 本程序的抽象

#pragma once
#define TITLE L"Title"
// 保证16:9
#define WNDWIDTH 1024
#define WNDHEIGHT 576

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
    // 检查彩色帧
    void check_color_frame();
    // 检查骨骼帧
    void check_body_frame();
    // 检查高清面部
    void check_hd_face_frame();
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
    // 面部模型构建器
    IFaceModelBuilder*                  m_pFaceModelBuilder = nullptr;
    // 面部模型顶点数量
    UINT                                m_cFaceVerticeCount = 0;
    // 面部模型是否被构建完毕
    UINT                                m_bProduced = FALSE;
    // 顶点缓存互斥锁
    std::mutex                          m_muxFaceVertice;
    // 退出
    std::atomic<BOOL>                   m_bExit = FALSE;
    // 骨骼帧
    IBody*                              m_apBody[BODY_COUNT];
private:
    // 面形
    float                               m_afFSD[FaceShapeDeformations_Count];
};
