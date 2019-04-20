// Author  : DustPG
// License : MIT: see more in "License.txt"

// 用途: 

// PrecisionTimer类 高精度计时器 仅可用于短时间计时 精度:μs级别
//		比如帧间时间、某函数耗时，用于长时间计时会因为各种原因丧失精度
//		老机器不支持 新机器多核、变频  总之 蛋疼

#pragma once

// 高精度计时器
class PrecisionTimer
{
public:
    // 更新频率 CPU变频的话影响颇大 可以手动刷新一次，比如每2秒更新一次
    void					RefreshFrequency(){ QueryPerformanceFrequency(&m_cpuFrequency); }
    // 开始计时
    void					Start(){ get_performance_counter(&m_cpuCounterStart); }
    // 起始时间赋值为结束时间 连续使用时请调用这个
    void					MovStartEnd(){ m_cpuCounterStart = m_cpuCounterEnd; }
    // 经过时间(秒) 返回单精度
    float					DeltaF_s(){
        get_performance_counter(&m_cpuCounterEnd);
        return (float)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*1e-3F / (float)m_cpuFrequency.QuadPart;
    }
    // 经过时间(秒) 返回双精度
    double					DeltaD_s(){
        get_performance_counter(&m_cpuCounterEnd);
        return (double)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*1e-3 / (double)m_cpuFrequency.QuadPart;
    }
    // 经过时间(毫秒) 返回单精度
    float					DeltaF_ms(){
        get_performance_counter(&m_cpuCounterEnd);
        return (float)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart) / (float)m_cpuFrequency.QuadPart;
    }
    // 经过时间(毫秒) 返回双精度
    double					DeltaD_ms(){
        get_performance_counter(&m_cpuCounterEnd);
        return (double)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart) / (double)m_cpuFrequency.QuadPart;
    }
    // 经过时间(微秒) 返回单精度
    float					DeltaF_mcs(){
        get_performance_counter(&m_cpuCounterEnd);
        return (float)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*1e3F / (float)m_cpuFrequency.QuadPart;
    }
    // 经过时间(微秒) 返回双精度
    double					DeltaD_mcs(){
        get_performance_counter(&m_cpuCounterEnd);
        return (double)(m_cpuCounterEnd.QuadPart - m_cpuCounterStart.QuadPart)*1e3 / (double)m_cpuFrequency.QuadPart;
    }
private:
    // CPU 当前频率
    LARGE_INTEGER			m_cpuFrequency;
    // CPU 开始计时时间
    LARGE_INTEGER			m_cpuCounterStart;
    // CPU 结束计时时间
    LARGE_INTEGER			m_cpuCounterEnd;
private:
    // 获取 CPU 执行计数 QueryPerformanceCounter的高性能版
    void get_performance_counter(LARGE_INTEGER* counter){
        DWORD dwLow, dwHigh;
        __asm
        {
            rdtsc;
            mov dwLow, eax;
            mov dwHigh, edx;
        }
        counter->HighPart = dwHigh;
        counter->LowPart = dwLow;
    }
public:
    // 强制单实例
    PrecisionTimer(){ RefreshFrequency(); }
    // 唯一实例
    //static	PrecisionTimer	s_instance;
};