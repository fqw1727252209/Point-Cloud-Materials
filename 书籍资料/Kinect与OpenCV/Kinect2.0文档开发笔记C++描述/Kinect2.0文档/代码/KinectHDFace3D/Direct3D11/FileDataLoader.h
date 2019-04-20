// FileDataLoader类 文件读取类

#pragma once


// FileDataLoader类
class FileDataLoader{
private:
    // 私有化构造函数
    FileDataLoader(){  }
    // 私有化析构函数
    ~FileDataLoader(){
        if (m_pData){
            free(m_pData);
            m_pData = nullptr;
        }
    }
public:
    // 读取文件
    bool ReadFile(WCHAR* file_name);
    // 获取数据指针
    const void* GetData(){ return m_pData; }
    // 获取数据长度
    size_t GetLength(){ return m_cLength; }
private:
    // 数据指针
    void*           m_pData = nullptr;
    // 数据长度
    size_t          m_cLength = 0;
    // 实际长度
    size_t          m_cLengthReal = 0;
public:
    // 渲染线程单例
    static FileDataLoader s_instanceForRenderThread;
};

#define FileLoader FileDataLoader::s_instanceForRenderThread