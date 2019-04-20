// ThisApp类 本程序的抽象

#pragma once

// ThisApp类
class ThisApp{
    // 名字真尼玛长 差评
    typedef NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE PROCESSOR_TYPE;
public:
    // 构造函数
    ThisApp();
    // 析构函数
    ~ThisApp();
    // 初始化
    HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
    // 消息循环
    void RunMessageLoop();
    // 重置
    void ResetReconstruction();
private:
    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查深度帧
    void check_depth_frame();
private:
    // 窗口句柄
    HWND                                    m_hwnd = nullptr;
    // Kinect v2 传感器
    IKinectSensor*                          m_pKinect = nullptr;
    // 深度帧读取器
    IDepthFrameReader*                      m_pDepthFrameReader = nullptr;
    // Kinect Fusion 容积重建
    INuiFusionReconstruction*               m_pReconstruction = nullptr;
    // 深度缓存
    UINT16*                                 m_pDepthImagePixelBuffer = nullptr;
    // 平滑前的浮点深度帧
    NUI_FUSION_IMAGE_FRAME*                 m_pDepthFloatImage = nullptr;
    // 平滑后的浮点深度帧
    NUI_FUSION_IMAGE_FRAME*                 m_pSmoothDepthFloatImage = nullptr;
    // 点云 Fusion 图像帧
    NUI_FUSION_IMAGE_FRAME*                 m_pPointCloud = nullptr;
    // 表面 Fusion 图像帧
    NUI_FUSION_IMAGE_FRAME*                 m_pSurfaceImageFrame = nullptr;
    // 法线 Fusion 图像帧
    NUI_FUSION_IMAGE_FRAME*                 m_pNormalImageFrame = nullptr;
    // 映射器
    ICoordinateMapper*                      m_pMapper = nullptr;
    // 深度空间点
    DepthSpacePoint*                        m_pDepthDistortionMap = nullptr;
    // 深度失真
    UINT*                                   m_pDepthDistortionLT = nullptr;
    // 彩色临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE                         m_hDepthFrameArrived = 0;
    // 坐标映射改变事件
    WAITABLE_HANDLE                         m_coordinateMappingChangedEvent = 0;
    // Kinect Fusion 相机转换
    Matrix4                                 m_worldToCameraTransform;
    // 默认Kinect Fusion 世界到容积转换
    Matrix4                                 m_defaultWorldToVolumeTransform;
    // Kinect Fusion 容积重建参数
    NUI_FUSION_RECONSTRUCTION_PARAMETERS    m_reconstructionParams;
    // Kinect Fusion 相机参数
    NUI_FUSION_CAMERA_PARAMETERS            m_cameraParameters;
    // 渲染器
    ImageRenderer                           m_ImagaRenderer;
    // 图像宽度
    UINT                                    m_cDepthWidth = NUI_DEPTH_RAW_WIDTH;
    // 图像高度
    UINT                                    m_cDepthHeight = NUI_DEPTH_RAW_HEIGHT;
    // Fusion处理器类型
    ThisApp::PROCESSOR_TYPE                 m_processorType;
    // 设备索引 处理设备使用默认的(-1)
    int                                     m_deviceIndex = -1;
    //
    USHORT                                  m_cMaxIntegrationWeight = NUI_FUSION_DEFAULT_INTEGRATION_WEIGHT;
    // 保留
    USHORT                                  unused = 0;
    // 计时器
    PrecisionTimer                          m_timer;
    // 深度最低阈值
    float                                   m_fMinDepthThreshold = NUI_FUSION_DEFAULT_MINIMUM_DEPTH;
    // 深度最远阈值
    float                                   m_fMaxDepthThreshold = NUI_FUSION_DEFAULT_MAXIMUM_DEPTH;
};