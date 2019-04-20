#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) (sizeof(a)/sizeof(*a))

#define THRESHOLD (0.1f)


// ThisApp构造函数
ThisApp::ThisApp(){
    ZeroMemory(m_apBodies, sizeof(m_apBodies));
    if (SUCCEEDED(init_kinect())){
        _cwprintf(L"初始化成功\n");
        while (true){
            // 骨骼临帧
            if (WaitForSingleObject(reinterpret_cast<HANDLE>(m_hBodyFrameArrived), 20) == WAIT_OBJECT_0){
                if (FAILED(this->check_body_frame()))
                    continue;
                // 在手势帧没有跟踪时常试更换id
                BOOLEAN tracked = TRUE;
                m_pGestureFrameSource->get_IsTrackingIdValid(&tracked);
                if (tracked) continue;
                UINT64 id = 0;
                for (int i = 0; i < BODY_COUNT; ++i){
                    m_apBodies[i]->get_IsTracked(&tracked);
                    if (tracked){
                        m_apBodies[i]->get_TrackingId(&id);
                        m_pGestureFrameSource->put_TrackingId(id);
                        _cwprintf(L"更换ID: %l64d\n", id);
                        break;
                    }
                }
            }
            // 获取最近一帧手势
            IVisualGestureBuilderFrame* pGestureFrame = nullptr;
            if (SUCCEEDED(m_pGestureFrameReader->CalculateAndAcquireLatestFrame(&pGestureFrame))){
                // 检查连续型手势
                IContinuousGestureResult* pContinuousGestureResult = nullptr;
                float progress = 0.f;
                // 循环检查
                for (UINT i = 0U; i < m_cGestureSize; ++i){
                    // 获取手势结果
                    pGestureFrame->get_ContinuousGestureResult(
                        m_apGestures[i],
                        &pContinuousGestureResult
                        );
                    if (pContinuousGestureResult){
                        // 显示—— 手势名称: 进度
                        WCHAR buffer[MAX_PATH];
                        HRESULT hr = m_apGestures[i]->get_Name(lengthof(buffer), buffer);
                        pContinuousGestureResult->get_Progress(&progress);
                        wprintf(L"%32s: progress: %3.03f\n", buffer, progress);
                        // 释放对象
                        pContinuousGestureResult->Release();
                        pContinuousGestureResult = nullptr;
                        // 超过范围则退出
                        if (progress > (1.f - THRESHOLD)){
                            goto end_of_life;
                        }
                    }
                }
            }
            SafeRelease(pGestureFrame);
        }
    }
end_of_life:
    return;
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    // 释放对象
    SafeRelease(m_pGestureFrameReader);
    // 释放手势
    for (UINT i = 0; i < m_cGestureSize; ++i) {
        SafeRelease(m_apGestures[i]);
    }
    // 释放骨骼数据
    for (int i = 0; i < BODY_COUNT; ++i){
        SafeRelease(m_apBodies[i]);
    }
    SafeRelease(m_pGestureFrameSource);
    SafeRelease(m_pGestureDatabase);
    SafeRelease(m_pKinect);
    //关闭事件
    if (m_hExit){
        ::CloseHandle(m_hExit);
        m_hExit = nullptr;
    }
}


// 音频线程
/*void ThisApp::AudioThread(ThisApp* pointer){
    // 先设置
    HANDLE events[] = { pointer->m_hExit, pointer->m_hSpeechEvent };
    bool exit = false;
    while (!exit) {
        switch (::WaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE)){
        case WAIT_OBJECT_0 + 0:
            // 退出程序
            exit = true;
            pointer->m_p16BitPCMAudioStream->SetSpeechState(FALSE);
            break;
        case WAIT_OBJECT_0 + 1:
            // 语言识别
            pointer->speech_process();
            break;
        }
    }
}*/

// 初始化Kinect
HRESULT ThisApp::init_kinect(){
#ifdef _DEBUG
    static bool first = false;
    if (first){
        ::MessageBoxW(nullptr, L"多次初始化", L"<ThisApp::init_kinect>", MB_ICONERROR);
    }
    first = true;
#endif
    IBodyFrameSource* pBodyFrameSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 载入手势
    if (SUCCEEDED(hr)){
        hr = CreateVisualGestureBuilderDatabaseInstanceFromFile(L"test.gbd", &m_pGestureDatabase);
    }
    // 获取骨骼帧源
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_BodyFrameSource(&pBodyFrameSource);
    }
    // 获取骨骼帧读取器
    if (SUCCEEDED(hr)){
        hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        hr = m_pBodyFrameReader->SubscribeFrameArrived(&m_hBodyFrameArrived);
    }
    // 创建手势帧源
    if (SUCCEEDED(hr)){
        hr = CreateVisualGestureBuilderFrameSource(m_pKinect, 0, &m_pGestureFrameSource);
    }
    // 检查手势
    if (SUCCEEDED(hr)){
        hr = m_pGestureDatabase->get_AvailableGesturesCount(&m_cGestureSize);
    }
#ifdef _DEBUG
    // 检查溢出
    if (m_cGestureSize > lengthof(m_apGestures)){
        assert(!"m_cGestureSize > lengthof(m_apGestures)");
        hr = DISP_E_BUFFERTOOSMALL;
    }
#endif
    // 获取手势
    if (SUCCEEDED(hr)){
        hr = m_pGestureDatabase->get_AvailableGestures(m_cGestureSize, m_apGestures);
    }
    // 添加手势
    if (SUCCEEDED(hr)){
        hr = m_pGestureFrameSource->AddGestures(m_cGestureSize, m_apGestures);
    }
    // 获取读取器
    if (SUCCEEDED(hr)){
        hr = m_pGestureFrameSource->OpenReader(&m_pGestureFrameReader);
    }
    SafeRelease(pBodyFrameSource);
    return hr;
}


// 检查骨骼帧
HRESULT ThisApp::check_body_frame(){
    // 骨骼临帧事件参数
    IBodyFrameArrivedEventArgs* pArgs = nullptr;
    // 骨骼帧引用
    IBodyFrameReference* pBFrameRef = nullptr;
    // 骨骼帧
    IBodyFrame* pBodyFrame = nullptr;

    // 获取参数
    HRESULT hr = m_pBodyFrameReader->GetFrameArrivedEventData(m_hBodyFrameArrived, &pArgs);
    // 获取引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pBFrameRef);
    }
    // 获取骨骼帧
    if (SUCCEEDED(hr)) {
        hr = pBFrameRef->AcquireFrame(&pBodyFrame);
    }
    // 获取骨骼数据
    if (SUCCEEDED(hr)) {
        hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, m_apBodies);
    }
    SafeRelease(pBodyFrame);
    SafeRelease(pBFrameRef);
    SafeRelease(pArgs);
    return hr;
}