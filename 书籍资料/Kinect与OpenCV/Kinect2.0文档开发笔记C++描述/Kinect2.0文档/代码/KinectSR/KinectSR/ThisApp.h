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
    // 退出
    void Exit(){ SetEvent(m_hExit); m_threadAudio.join(); }
private:
    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 初始化语音识别
    HRESULT init_speech_recognizer();
    // 音频线程
    static void AudioThread(ThisApp* pointer);
    // 语音行为
    void speech_behavior(const SPPHRASEPROPERTY* tag);
    // 语音处理
    void speech_process();
private:
    // Kinect v2 传感器
    IKinectSensor*              m_pKinect = nullptr;
    // 包装器
    KinectAudioStreamWrapper*   m_p16BitPCMAudioStream = nullptr;
    // 音频
    IAudioBeam*                 m_pAudioBeam = nullptr;
    // 语音识别输入流
    ISpStream*                  m_pSpeechStream = nullptr;
    // 语音识别器
    ISpRecognizer*              m_pSpeechRecognizer = nullptr;
    // 语音识别上下文
    ISpRecoContext*             m_pSpeechContext = nullptr;
    // 语音识别语法
    ISpRecoGrammar*             m_pSpeechGrammar = nullptr;
    // 语音识别触发事件
    HANDLE                      m_hSpeechEvent = nullptr;
    // 退出事件
    HANDLE                      m_hExit = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    // 音频处理线程
    std::thread                 m_threadAudio;
    // 语法文件
    static WCHAR*               s_GrammarFileName;
};