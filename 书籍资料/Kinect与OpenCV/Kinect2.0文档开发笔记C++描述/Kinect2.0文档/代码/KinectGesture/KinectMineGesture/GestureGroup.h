// GestureGroup类 组合手势类 粗略表示一连续手势

#pragma once



// GestureGroup类
class GestureGroup : public GestureBase{
    struct Group{
        // 指针
        GestureStateBase*   pointer;
        // 至少时间
        UINT16              least;
        // 至多时间
        UINT16              most;
    };
public:
    // 构造函数
    GestureGroup(std::initializer_list<Group>& list, wchar_t* name);
    // 删除默认构造
    GestureGroup() = delete;
    // 析构函数
    virtual ~GestureGroup();
    // 刷新
    virtual bool Updata(GestureDataRequired* data);
    // 获取类型
    virtual GestureType GetType() { return GestureType::TYPE_GROUP; };
    // 获取手势名称
    virtual wchar_t* GetName(){ return m_name; };
private:
    // 数据指针
    Group*                  m_pGroup = nullptr;
    // 数据量
    UINT                    m_cMemberCount = 0;
    // 当前进度 为尝试进度
    UINT                    m_index = 0;
    // 时间单位
    DWORD                   m_dwTime = 0;
    // 手势名称
    wchar_t                 m_name[64];
};
