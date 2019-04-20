#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) (sizeof(a)/sizeof(*a))

#define SRLANGUAGE L""


// ThisApp构造函数
ThisApp::ThisApp(){
    if (SUCCEEDED(init_kinect()) && SUCCEEDED(init_speech_recognizer())){
        m_threadAudio.std::thread::~thread();
        m_threadAudio.std::thread::thread(AudioThread, this);
    }
    else{
        m_threadAudio.std::thread::~thread();
        m_threadAudio.std::thread::thread(AudioThread, this);
    }
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    // 释放语音识别对象
    SafeRelease(m_pSpeechGrammar);
    SafeRelease(m_pSpeechContext);
    SafeRelease(m_pSpeechRecognizer);
    SafeRelease(m_pSpeechStream);
    // 关闭包装器
    SafeRelease(m_p16BitPCMAudioStream);
    SafeRelease(m_pAudioBeam);
    SafeRelease(m_pKinect);
    //关闭事件
    if (m_hExit){
        ::CloseHandle(m_hExit);
        m_hExit = nullptr;
    }
    if (m_hSpeechEvent){
        // SR会关闭的
        //::CloseHandle(m_hSpeechEvent);
        m_hSpeechEvent = nullptr;
    }
}


// 音频线程
void ThisApp::AudioThread(ThisApp* pointer){
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
}

// 初始化Kinect
HRESULT ThisApp::init_kinect(){
    IAudioSource* pAudioSource = nullptr;
    IAudioBeamList* pAudioBeamList = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 获取音频源
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_AudioSource(&pAudioSource);
    }
    // 获取音频链表
    if (SUCCEEDED(hr)){
        hr = pAudioSource->get_AudioBeams(&pAudioBeamList);
    }
    // 获取音频
    if (SUCCEEDED(hr)){
        hr = pAudioBeamList->OpenAudioBeam(0, &m_pAudioBeam);
    }
    // 获取输入音频流
    if (SUCCEEDED(hr)){
        IStream* pStream = nullptr;
        hr = m_pAudioBeam->OpenInputStream(&pStream);
        // 利用傀儡生成包装对象
        m_p16BitPCMAudioStream = new KinectAudioStreamWrapper(pStream);
        SafeRelease(pStream);
    }
    SafeRelease(pAudioBeamList);
    SafeRelease(pAudioSource);
    return hr;
}


// 初始化语音识别
HRESULT ThisApp::init_speech_recognizer(){
    HRESULT hr = S_OK;
    // 创建语音输入流
    if (SUCCEEDED(hr)){
        hr = CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), (void**)&m_pSpeechStream);;
    }
    // 与我们的Kinect语音输入相连接
    if (SUCCEEDED(hr)){
        WAVEFORMATEX wft = {
            WAVE_FORMAT_PCM, // PCM编码
            1, // 单声道
            16000,  // 采样率为16KHz
            32000, // 每分钟数据流 = 采样率 * 对齐
            2, // 对齐 : 单声道 * 样本深度 = 2byte
            16, // 样本深度 16BIT
            0 // 额外数据
        };
        // 设置状态
        hr = m_pSpeechStream->SetBaseStream(m_p16BitPCMAudioStream, SPDFID_WaveFormatEx, &wft);
    }
    // 创建语音识别对象
    if (SUCCEEDED(hr)){
        ISpObjectToken *pEngineToken = nullptr;
        // 创建语言识别器
        hr = CoCreateInstance(CLSID_SpInprocRecognizer, nullptr, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), (void**)&m_pSpeechRecognizer);
        if (SUCCEEDED(hr)) {
            // 连接我们创建的语音输入流对象
            m_pSpeechRecognizer->SetInput(m_pSpeechStream, TRUE);
            // 创建待识别语言 这里选择大陆汉语(zh-cn) 
            // 目前没有Kinect的汉语语音识别包 有的话可以设置"language=804;Kinect=Ture"
            hr = SpFindBestToken(SPCAT_RECOGNIZERS, L"Language=804", nullptr, &pEngineToken);
            if (SUCCEEDED(hr)) {
                // 设置待识别语言
                m_pSpeechRecognizer->SetRecognizer(pEngineToken);
                // 创建语音识别上下文
                hr = m_pSpeechRecognizer->CreateRecoContext(&m_pSpeechContext);
                // 适应性 ON! 防止因长时间的处理而导致识别能力的退化
                if (SUCCEEDED(hr))  {
                    hr = m_pSpeechRecognizer->SetPropertyNum(L"AdaptationOn", 0);
                }
            }
        }
        SafeRelease(pEngineToken);
    }
    // 创建语法
    if (SUCCEEDED(hr)){
        hr = m_pSpeechContext->CreateGrammar(1, &m_pSpeechGrammar);
    }
    // 加载静态SRGS语法文件
    if (SUCCEEDED(hr)){
        hr = m_pSpeechGrammar->LoadCmdFromFile(s_GrammarFileName, SPLO_STATIC);
    }
    // 激活语法规则
    if (SUCCEEDED(hr)){
        hr = m_pSpeechGrammar->SetRuleState(nullptr, nullptr, SPRS_ACTIVE);
    }
    // 设置识别器一直读取数据
    if (SUCCEEDED(hr)){
        hr = m_pSpeechRecognizer->SetRecoState(SPRST_ACTIVE_ALWAYS);
    }
    // 设置对识别事件感兴趣
    if (SUCCEEDED(hr)){
        hr = m_pSpeechContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));
    }
    // 保证语音识别处于激活状态
    if (SUCCEEDED(hr)){
        hr = m_pSpeechContext->Resume(0);
    }
    // 获取识别事件
    if (SUCCEEDED(hr)){
        m_p16BitPCMAudioStream->SetSpeechState(TRUE);
        m_hSpeechEvent = m_pSpeechContext->GetNotifyEventHandle();
    }
#ifdef _DEBUG
    else
        printf_s("init_speech_recognizer failed\n");
#endif
    return hr;
}


// 音频处理
void ThisApp::speech_process() {
    // 置信阈值
    const float ConfidenceThreshold = 0.3f;

    SPEVENT curEvent = { SPEI_UNDEFINED, SPET_LPARAM_IS_UNDEFINED, 0, 0, 0, 0 };
    ULONG fetched = 0;
    HRESULT hr = S_OK;
    // 获取事件
    m_pSpeechContext->GetEvents(1, &curEvent, &fetched);
    while (fetched > 0)
    {
        // 确定是识别事件
        switch (curEvent.eEventId)
        {
        case SPEI_RECOGNITION:
            // 保证位对象
            if (SPET_LPARAM_IS_OBJECT == curEvent.elParamType) {
                ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);
                SPPHRASE* pPhrase = nullptr;
                // 获取识别短语
                hr = result->GetPhrase(&pPhrase);
                if (SUCCEEDED(hr)) {
#ifdef _DEBUG
                    // DEBUG时显示识别字符串
                    WCHAR* pwszFirstWord;
                    result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszFirstWord, nullptr);
                    _cwprintf(pwszFirstWord);
                    ::CoTaskMemFree(pwszFirstWord);
#endif
                    pPhrase->pProperties;
                    const SPPHRASEELEMENT* pointer = pPhrase->pElements + 1;
                    if ((pPhrase->pProperties != nullptr) && (pPhrase->pProperties->pFirstChild != nullptr)) {
                        const SPPHRASEPROPERTY* pSemanticTag = pPhrase->pProperties->pFirstChild;
#ifdef _DEBUG
                        _cwprintf(L"   置信度:%d%%\n", (int)(pSemanticTag->SREngineConfidence*100.f));
#endif
                        if (pSemanticTag->SREngineConfidence > ConfidenceThreshold) {
                            speech_behavior(pSemanticTag);
                        }
                    }
                    ::CoTaskMemFree(pPhrase);
                }
            }
            break;
        }

        m_pSpeechContext->GetEvents(1, &curEvent, &fetched);
    }

    return;
}


// 语音行为
void ThisApp::speech_behavior(const SPPHRASEPROPERTY* tag){
    if (!tag) return;
    if (!wcscmp(tag->pszName, L"战况")){
        enum class Subject{
            US = 0,
            Enemy
        } ;
        enum class Predicate{
            Destroy = 0,
            Defeat,
            Breakdown
        };
        // 分析战况
        union  Situation{
            struct{
                // 主语
                Subject subject;
                // 谓语
                Predicate predicate;
                // 对象
                int object2;
                // 宾语
                int object;

            };
            UINT32 data[4];
        };
        Situation situation;
        auto obj = tag->pFirstChild;
        auto pointer = situation.data;
        // 填写数据
        while (obj) {
            *pointer = obj->vValue.lVal;
            ++pointer;
            obj = obj->pNextSibling;
        }
        // XXX
    }
    else if (!wcscmp(tag->pszName, L"发现东西")){
        // 发现东西
    }
}