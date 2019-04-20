// GestureStateBase类 状态手势类 表示状态的一种手势

#pragma once

// GestureStateBase 类
class GestureStateBase : public GestureBase{
public:
    // 获取类型
    virtual GestureType GetType(){ return GestureType::TYPE_STATE; };
    // 获取手势名称
    virtual wchar_t* GetName(){ return L"GestureState"; };
};


// GestureStateData
struct GestureStateData{
    // 结构体大小
    UINT    size = sizeof(GestureStateData);
    // 关节坐标
    Joint   joint[JointType_Count];
    // 浮动范围
    FLOAT   float_range[JointType_Count];
    // 权重
    FLOAT   weight[JointType_Count];
    // 手势名称
    WCHAR   name[64];
};


// GestureState 类
class GestureState : public GestureStateBase {
public:
    // 状态文件构造
    GestureState(wchar_t* file_name);
    // 内存构造
    GestureState(GestureStateData* address);
    // 删除默认构造函数
    GestureState() = delete;
    // 刷新
    virtual bool Update(GestureDataRequired*);
    // 获取手势名称
    virtual wchar_t* GetName(){ return m_data.name; };
private:
    // 手势数据
    GestureStateData        m_data;
};