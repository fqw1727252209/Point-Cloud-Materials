#include "stdafx.h"
#include "included.h"


// GestureGroup类构造函数
GestureGroup::GestureGroup(std::initializer_list<GestureGroup::Group>& list, wchar_t* name){
    m_cMemberCount = list.size();
    m_pGroup = new GestureGroup::Group[m_cMemberCount];
    auto pGroup = m_pGroup;
    for (auto& i : list){
        *pGroup = i;
        ++pGroup;
    }
    wcscpy_s(m_name, name);
    GESTURE_GET_TIME(m_dwTime);
}



// GestureGroup类 析构函数
GestureGroup::~GestureGroup(){
    if (m_pGroup){
        delete[] m_pGroup;
        m_pGroup = nullptr;
        m_cMemberCount = 0;
        m_index = 0;
    }
}


// 刷新
bool GestureGroup::Updata(GestureDataRequired* data){
#ifdef _DEBUG
    if (!m_pGroup) 
        ::MessageBoxW(nullptr, L"你在逗我", L"<GestureGroup::Updata> called.", MB_ICONERROR);
#endif
    Group group = m_pGroup[m_cMemberCount];
    // 刷新手势
    if (group.pointer->Update(data)){
        // 获取时间
        DWORD dwNowTime;
        GESTURE_GET_TIME(dwNowTime);
        // 第二个及之后超时重置
        if (dwNowTime - m_dwTime > group.most && !m_cMemberCount){
            m_cMemberCount = 0;
            m_dwTime = dwNowTime;
            return false;
        }
        // 在标准内
        if (dwNowTime - m_dwTime > group.least ){
            m_dwTime = dwNowTime;
            if (++m_cMemberCount > m_cMemberCount)
                return true;
        }
    }
    return false;
}